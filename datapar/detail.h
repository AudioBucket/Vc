/*  This file is part of the Vc library. {{{
Copyright © 2016 Matthias Kretz <kretz@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the names of contributing organizations nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

}}}*/

#ifndef VC_DATAPAR_DETAIL_H_
#define VC_DATAPAR_DETAIL_H_

namespace Vc_VERSIONED_NAMESPACE::detail
{
/**
 * \internal
 * Defines the implementation of a given <T, Abi>.
 */
template <class T, class Abi> struct traits;

template <class T> struct get_impl;
template <class T> using get_impl_t = typename get_impl<T>::type;

/**
 * \internal
 * Returns the next power of 2 larger than or equal to \p x.
 */
static constexpr std::size_t next_power_of_2(std::size_t x)
{
    return (x & (x - 1)) == 0 ? x : next_power_of_2((x | (x >> 1)) + 1);
}

/**
 * \internal
 * Tag used for private init constructor of datapar and mask
 */
static constexpr struct private_init_t {} private_init = {};

template <class L, class R> struct return_type_impl;
template <class L> struct return_type_impl<L, L> {
    using type = L;
};

template <class L, class R> using return_type = typename return_type_impl<L, R>::type;
template <class L, class R> using cmp_return_type = typename return_type<L, R>::mask_type;
}  // namespace Vc_VERSIONED_NAMESPACE::detail
#endif  // VC_DATAPAR_DETAIL_H_

// vim: foldmethod=marker