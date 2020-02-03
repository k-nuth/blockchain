// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_UTILITY_TUPLE_ELEMENT_HPP_
#define KTH_UTILITY_TUPLE_ELEMENT_HPP_

#include <type_traits>
#include <tuple>

// For Transitioning from C++11 to C++14
#if __cpp_lib_tuple_element_t == 201402

namespace bitprim {

using std::tuple_element_t;

} // namespace bitprim

#else 

namespace bitprim {

template <std::size_t I, typename T>
using tuple_element_t = typename std::tuple_element<I, T>::type;

} // namespace bitprim

#endif  // __cpp_lib_tuple_element_t == 201402

#endif //KTH_UTILITY_TUPLE_ELEMENT_HPP_
