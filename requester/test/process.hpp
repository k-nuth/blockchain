#ifndef BITPRIM_TEST_PROCESS_HPP
#define BITPRIM_TEST_PROCESS_HPP

#include <cassert>
#include <cstdlib>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cerrno>
#  include <fcntl.h>
#  include <signal.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

//! Mechanism to represent and perform operations on a process
class process {
public:
    //! Creates new process object which does not represent a process.
    //! \postconditions joinable() is false
    process() noexcept
      : _handle(0)
    {}

    //! Constructs an object of type process from rhs, and sets rhs to a
    //! default constructed state.
    //! \postconditions rhs.joinable() is false
    process(process&& rhs) noexcept
      : _handle(rhs._handle)
    {
        rhs._handle = 0;
    }

    //! \requires joinable() is false
    //! \note a process must be either joined or detached before destruction
    ~process()
    {
        assert(!joinable());
    }

    //! Assigns the state of rhs to *this and sets rhs to a default
    //! constructed state.
    //! \requires joinable() is false
    process& operator=(process&& rhs) noexcept
    {
        assert(!joinable());
        _handle = rhs._handle;
        rhs._handle = 0;
        return *this;
    }

    //! \returns: whether *this represents a process
    bool joinable() const noexcept
    {
        return _handle != 0;
    }

    //! Sends a SIGTERM signal to the process represented by *this.
    //! \requires joinable() is true
    void terminate()
    {
        assert(joinable());

#ifdef _WIN32
        if (::TerminateProcess(_handle, 0) == 0)
        {
            throw std::system_error(::GetLastError(), std::system_category());
        }
#else
        if (::kill(_handle, SIGTERM) == -1)
        {
            throw std::system_error(errno, std::generic_category());
        }
#endif
    }

    //! Blocks until the process represented by *this has completed.
    //! \requires joinable() is true
    //! \postconditions joinable() is false
    void join()
    {
        assert(joinable());

#ifdef _WIN32
        if (::WaitForSingleObject(_handle, INFINITE) == WAIT_FAILED)
        {
            throw std::system_error(::GetLastError(), std::system_category());
        }
#else
        int status = 0;
        if (::waitpid(_handle, &status, 0) == -1)
        {
            throw std::system_error(errno, std::generic_category());
        }
#endif

        _handle = 0;
    }

    //! The process represented by *this continues execution without the
    //! calling thread blocking. When detach() returns, *this no longer
    //! represents the possibly continuing process.
    //! \requires joinable() is true
    //! \postconditions joinable() is false
    void detach()
    {
        assert(joinable());
        _handle = 0;
    }

    //! Constructs an object of type process. The new process executes
    //! the command path with the given arguments.
    static process exec(
        std::string const& path,
        std::vector<std::string> const& args)
    {
#ifdef _WIN32
        std::string command_line = [&] {
            std::string command_line;
            command_line += path.c_str();
            for (std::string const& arg : args)
                command_line += ' ' + arg;
            return command_line;
        }();

        PROCESS_INFORMATION process_info = {};
        STARTUPINFO startup_info = { sizeof(STARTUPINFO) };
        if (::CreateProcessA(nullptr, &command_line[0], nullptr, nullptr, FALSE,
                0, nullptr, nullptr, &startup_info, &process_info) == 0)
        {
            ::CloseHandle(process_info.hProcess);
            ::CloseHandle(process_info.hThread);
            throw std::system_error(::GetLastError(), std::system_category());
        }

        HANDLE handle = process_info.hProcess;
#else
        std::vector<char const*> const argv = [&] {
            std::vector<char const*> argv;
            argv.reserve(args.size() + 2);
            argv.push_back(path.c_str());
            for (std::string const& arg : args)
                argv.push_back(arg.c_str());
            argv.push_back(nullptr);
            return argv;
        }();

        int eout[2];
        ::pipe(eout);
        pid_t const handle = ::fork();
        if (handle == -1)
        {
            throw std::system_error(errno, std::generic_category());
        } else if (handle == 0) { // child
            ::close(eout[0]);
            ::fcntl(eout[1], F_SETFD, FD_CLOEXEC);
            ::execvp(path.c_str(), const_cast<char* const*>(argv.data()));

            int const error = errno;
            ::write(eout[1], &error, sizeof(error));
            std::exit(EXIT_FAILURE);
        }
        ::close(eout[1]);

        int error = 0;
        if (::read(eout[0], &error, sizeof(error)) != 0)
        {
            throw std::system_error(error, std::generic_category());
        }
#endif

        process child;
        child._handle = handle;
        return child;
    }

    static process exec(std::string const& path)
    {
        return exec(path, {});
    }

private:
#ifdef _WIN32
    HANDLE _handle;
#else
    pid_t _handle;
#endif
};

#endif /*BITPRIM_TEST_PROCESS_HPP*/
