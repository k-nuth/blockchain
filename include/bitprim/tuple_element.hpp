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

#ifndef BITPRIM_UTILITY_TUPLE_ELEMENT_HPP_
#define BITPRIM_UTILITY_TUPLE_ELEMENT_HPP_

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

#endif //BITPRIM_UTILITY_TUPLE_ELEMENT_HPP_
