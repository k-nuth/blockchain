/**
 * Copyright (c) 2016-2018 Bitprim Inc.
 *
 * This file is part of Bitprim.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BITPRIM_BLOCKCHAIN_MINING_PRIORITIZER_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_PRIORITIZER_HPP_


// #include <thread>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

// #include <boost/thread/condition_variable.hpp>
// #include <boost/thread/locks.hpp>
// #include <boost/thread/shared_mutex.hpp>

namespace libbitcoin {
namespace mining {

// template <typename Mutex>
class prioritizer {
public:
    using unique_lock_t = std::unique_lock<std::mutex>;
    using lock_guard_t = std::lock_guard<std::mutex>;

    prioritizer() = default;

#ifndef NDEBUG    
    ~prioritizer() { 
        assert(!waiting_); 
    }
#endif

    prioritizer(prioritizer const&) = delete;
    prioritizer operator=(prioritizer const&) = delete;

    template <typename F>
    auto low_job(F f) const {
        unique_lock_t lk(gate_);    //Locked
        cv_.wait(lk, [&]{ return ! waiting_; });

        auto res = f();

        // cv_.notify_all();
        cv_.notify_one();

        return res;
    }

    template <typename F>
    auto high_job(F f) const {
        waiting_ = true;
        lock_guard_t lk(gate_);
        
        waiting_ = false;

        auto res = f();

        // cv_.notify_all();
        cv_.notify_one();

        return res;
    }

private:
    mutable std::condition_variable cv_;
    mutable std::mutex gate_;
            //shared_mutex
    mutable std::atomic<bool> waiting_ {false};
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_PRIORITIZER_HPP_
