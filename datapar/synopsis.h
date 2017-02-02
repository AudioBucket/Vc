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

#ifndef VC_DATAPAR_SYNOPSIS_H_
#define VC_DATAPAR_SYNOPSIS_H_

#include "../common/macros.h"
#include "../common/declval.h"
#include "macros.h"
#include "detail.h"
#include "where.h"

Vc_VERSIONED_NAMESPACE_BEGIN
namespace datapar_abi
{
constexpr int max_fixed_size = 32;
template <int N> struct fixed_size {};
using scalar = fixed_size<1>;
struct sse {};
struct avx {};
struct avx512 {};
struct knc {};

template <int N> struct partial_sse {};
template <int N> struct partial_avx {};
template <int N> struct partial_avx512 {};
template <int N> struct partial_knc {};

namespace detail
{
template <class T, class A0, class A1> struct fallback_abi_for_long_double {
    using type = A0;
};
template <class A0, class A1> struct fallback_abi_for_long_double<long double, A0, A1> {
    using type = A1;
};
template <class T, class A0, class A1>
using fallback_abi_for_long_double_t =
    typename fallback_abi_for_long_double<T, A0, A1>::type;
}  // namespace detail

#if defined Vc_IS_AMD64
#if !defined Vc_HAVE_SSE2
#error "Use of SSE2 is required on AMD64"
#endif
template <typename T>
using compatible = detail::fallback_abi_for_long_double_t<T, sse, scalar>;
#elif defined Vc_HAVE_FULL_KNC_ABI
template <typename T>
using compatible = detail::fallback_abi_for_long_double_t<T, knc, scalar>;
#else
template <typename> using compatible = scalar;
#endif

#if defined Vc_HAVE_FULL_AVX512_ABI
template <typename T>
using native = detail::fallback_abi_for_long_double_t<T, avx512, scalar>;
#elif defined Vc_HAVE_AVX512_ABI
template <typename T>
using native =
    std::conditional_t<(sizeof(T) >= 4),
                       detail::fallback_abi_for_long_double_t<T, avx512, scalar>, avx>;
#elif defined Vc_HAVE_FULL_AVX_ABI
template <typename T> using native = detail::fallback_abi_for_long_double_t<T, avx, scalar>;
#elif defined Vc_HAVE_AVX_ABI
template <typename T>
using native =
    std::conditional_t<std::is_floating_point<T>::value,
                       detail::fallback_abi_for_long_double_t<T, avx, scalar>, sse>;
#elif defined Vc_HAVE_FULL_SSE_ABI
template <typename T>
using native = detail::fallback_abi_for_long_double_t<T, sse, scalar>;
#elif defined Vc_HAVE_SSE_ABI
template <typename T>
using native = std::conditional_t<std::is_same<float, T>::value, sse, scalar>;
#elif defined Vc_HAVE_FULL_KNC_ABI
template <typename T>
using native = detail::fallback_abi_for_long_double_t<T, knc, scalar>;
#else
template <typename> using native = scalar;
#endif
}  // namespace datapar_abi

template <class T> struct is_datapar : public std::false_type {};
template <class T> constexpr bool is_datapar_v = is_datapar<T>::value;

template <class T> struct is_mask : public std::false_type {};
template <class T> constexpr bool is_mask_v = is_mask<T>::value;

template <class T, class Abi = datapar_abi::compatible<T>>
struct datapar_size
    : public std::integral_constant<size_t, detail::traits<T, Abi>::size()> {
};
template <class T, class Abi = datapar_abi::compatible<T>>
constexpr size_t datapar_size_v = datapar_size<T, Abi>::value;

namespace detail
{
template <class T, size_t N, bool, bool> struct abi_for_size_impl;
template <class T, size_t N> struct abi_for_size_impl<T, N, true, true> {
    using type = datapar_abi::fixed_size<N>;
};
template <class T> struct abi_for_size_impl<T, 1, true, true> {
    using type = datapar_abi::scalar;
};
#ifdef Vc_HAVE_SSE_ABI
template <> struct abi_for_size_impl<float, 4, true, true> { using type = datapar_abi::sse; };
#endif
#ifdef Vc_HAVE_FULL_SSE_ABI
template <> struct abi_for_size_impl<double, 2, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl< llong, 2, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl<ullong, 2, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl<  long, 16 / sizeof(long), true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl< ulong, 16 / sizeof(long), true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl<   int, 4, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl<  uint, 4, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl< short, 8, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl<ushort, 8, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl< schar, 16, true, true> { using type = datapar_abi::sse; };
template <> struct abi_for_size_impl< uchar, 16, true, true> { using type = datapar_abi::sse; };
#endif
#ifdef Vc_HAVE_AVX_ABI
template <> struct abi_for_size_impl<double, 4, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<float, 8, true, true> { using type = datapar_abi::avx; };
#endif
#ifdef Vc_HAVE_FULL_AVX_ABI
template <> struct abi_for_size_impl< llong,  4, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<ullong,  4, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<  long, 32 / sizeof(long), true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl< ulong, 32 / sizeof(long), true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<   int,  8, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<  uint,  8, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl< short, 16, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl<ushort, 16, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl< schar, 32, true, true> { using type = datapar_abi::avx; };
template <> struct abi_for_size_impl< uchar, 32, true, true> { using type = datapar_abi::avx; };
#endif
#ifdef Vc_HAVE_AVX512_ABI
template <> struct abi_for_size_impl<double, 8, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<float, 16, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl< llong,  8, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<ullong,  8, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<  long, 64 / sizeof(long), true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl< ulong, 64 / sizeof(long), true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<   int, 16, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<  uint, 16, true, true> { using type = datapar_abi::avx512; };
#endif
#ifdef Vc_HAVE_FULL_AVX512_ABI
template <> struct abi_for_size_impl< short, 32, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl<ushort, 32, true, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl< schar, 64, false, true> { using type = datapar_abi::avx512; };
template <> struct abi_for_size_impl< uchar, 64, false, true> { using type = datapar_abi::avx512; };
// fixed_size must support 64 entries because schar and uchar have 64 entries. Everything in
// between max_fixed_size and 64 doesn't have to be supported.
template <class T> struct abi_for_size_impl<T, 64, false, true> {
    using type = datapar_abi::fixed_size<64>;
};
#endif
#ifdef Vc_HAVE_FULL_KNC_ABI
template <class T> struct abi_for_size_impl<T, datapar_size_v<T, datapar_abi::knc>, true, true> {
    using type = datapar_abi::knc;
};
#endif
}  // namespace detail
template <class T, size_t N>
struct abi_for_size
    : public detail::abi_for_size_impl<T, N, (N <= datapar_abi::max_fixed_size),
                                       std::is_arithmetic<T>::value> {
};
template <size_t N> struct abi_for_size<bool, N> {
};
template <class T> struct abi_for_size<T, 0> {
};
template <class T, size_t N> using abi_for_size_t = typename abi_for_size<T, N>::type;

template <class T, class U = typename T::value_type>
struct memory_alignment
    : public std::integral_constant<size_t,
                                    detail::next_power_of_2(sizeof(U) * T::size())> {
};
template <class T, class U = typename T::value_type>
constexpr size_t memory_alignment_v = memory_alignment<T, U>::value;

// class template datapar [datapar]
template <class T, class Abi = datapar_abi::compatible<T>> class datapar;
template <class T, class Abi> struct is_datapar<datapar<T, Abi>> : public std::true_type {};
template <class T> using native_datapar = datapar<T, datapar_abi::native<T>>;
template <class T, int N> using fixed_size_datapar = datapar<T, datapar_abi::fixed_size<N>>;

// class template mask [mask]
template <class T, class Abi = datapar_abi::compatible<T>> class mask;
template <class T, class Abi> struct is_mask<mask<T, Abi>> : public std::true_type {};
template <class T> using native_mask = mask<T, datapar_abi::native<T>>;
template <class T, int N> using fixed_size_mask = mask<T, datapar_abi::fixed_size<N>>;

namespace detail
{
template <class T, class Abi> struct get_impl<Vc::mask<T, Abi>> {
    using type = typename traits<T, Abi>::mask_impl_type;
};
template <class T, class Abi> struct get_impl<Vc::datapar<T, Abi>> {
    using type = typename traits<T, Abi>::datapar_impl_type;
};
}  // namespace detail

// compound assignment [datapar.cassign]
template <class T, class Abi, class U> datapar<T, Abi> &operator +=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator -=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator *=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator /=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator %=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator &=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator |=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator ^=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator<<=(datapar<T, Abi> &, const U &);
template <class T, class Abi, class U> datapar<T, Abi> &operator>>=(datapar<T, Abi> &, const U &);

// binary operators [datapar.binary]
#define Vc_OP(op_, fun_) \
template <class T, class A> datapar<T, A> operator op_(const datapar<T, A> &x, const datapar<T, A> &y)     { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> datapar<T, A> operator op_(const datapar<T, A> &x, const detail::id<datapar<T, A>> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> datapar<T, A> operator op_(const detail::id<datapar<T, A>> &x, const datapar<T, A> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); }
Vc_OP(+, plus)
Vc_OP(-, minus)
Vc_OP(*, multiplies)
Vc_OP(/, divides)
#undef Vc_OP

#define Vc_OP(op_, fun_) \
template <class T, class A> enable_if<std::is_integral<T>::value, datapar<T, A>> operator op_(const datapar<T, A> &x, const datapar<T, A> &y)     { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> enable_if<std::is_integral<T>::value, datapar<T, A>> operator op_(const datapar<T, A> &x, const detail::id<datapar<T, A>> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> enable_if<std::is_integral<T>::value, datapar<T, A>> operator op_(const detail::id<datapar<T, A>> &x, const datapar<T, A> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); }
Vc_OP(%, modulus)
Vc_OP(&, bit_and)
Vc_OP(|, bit_or)
Vc_OP(^, bit_xor)
Vc_OP(<<, bit_shift_left)
Vc_OP(>>, bit_shift_right)
#undef Vc_OP

// compares [datapar.comparison]
#define Vc_OP(op_, fun_) \
template <class T, class A> mask<T, A> operator op_(const datapar<T, A> &x, const datapar<T, A> &y)     { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> mask<T, A> operator op_(const datapar<T, A> &x, const detail::id<datapar<T, A>> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); } \
template <class T, class A> mask<T, A> operator op_(const detail::id<datapar<T, A>> &x, const datapar<T, A> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(x, y); }
Vc_OP(==, equal_to)
Vc_OP(!=, not_equal_to)
Vc_OP(< , less)
Vc_OP(<=, less_equal)
#undef Vc_OP
#define Vc_OP(op_, fun_) \
template <class T, class A> mask<T, A> operator op_(const datapar<T, A> &x, const datapar<T, A> &y)     { return detail::get_impl_t<datapar<T, A>>::fun_(y, x); } \
template <class T, class A> mask<T, A> operator op_(const datapar<T, A> &x, const detail::id<datapar<T, A>> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(y, x); } \
template <class T, class A> mask<T, A> operator op_(const detail::id<datapar<T, A>> &x, const datapar<T, A> &y) { return detail::get_impl_t<datapar<T, A>>::fun_(y, x); }
Vc_OP(> , less)
Vc_OP(>=, less_equal)
#undef Vc_OP

// casts [datapar.casts]
#if defined Vc_CXX17
template <class T, class U, class... Us, size_t NN = U::size() + Us::size()...>
inline std::conditional_t<(T::size() == NN), T, std::array<T, NN / T::size()>>
    datapar_cast(U, Us...);
#endif

// mask binary operators [mask.binary]
#define Vc_OP(op_, fun_) \
template <class T, class A> mask<T, A> operator op_(const mask<T, A> &x, const mask<T, A> &y)     { return detail::get_impl_t<mask<T, A>>::fun_(x, y); } \
template <class T, class A> mask<T, A> operator op_(const mask<T, A> &x, const detail::id<mask<T, A>> &y) { return detail::get_impl_t<mask<T, A>>::fun_(x, y); } \
template <class T, class A> mask<T, A> operator op_(const detail::id<mask<T, A>> &x, const mask<T, A> &y) { return detail::get_impl_t<mask<T, A>>::fun_(x, y); }
Vc_OP(&&, logical_and)
Vc_OP(||, logical_or)
Vc_OP(& , bit_and)
Vc_OP(| , bit_or)
Vc_OP(^ , bit_xor)
#undef Vc_OP

// mask compares [mask.comparison]
template <class T0, class A0, class T1, class A1>
inline std::enable_if_t<
    disjunction<std::is_convertible<mask<T0, A0>, mask<T1, A1>>,
                std::is_convertible<mask<T1, A1>, mask<T0, A0>>>::value,
    bool>
operator==(const mask<T0, A0> &x, const mask<T1, A1> &y)
{
    return std::equal_to<mask<T0, A0>>{}(x, y);
}
template <class T0, class A0, class T1, class A1>
inline auto operator!=(const mask<T0, A0> &x, const mask<T1, A1> &y)
{
    return !operator==(x, y);
}

// reductions [mask.reductions]
template <class T, class Abi> inline bool all_of(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = 0; i < N; ++i) {
        if (!k[i]) {
            return false;
        }
    }
    return true;
}

template <class T, class Abi> inline bool any_of(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = 0; i < N; ++i) {
        if (k[i]) {
            return true;
        }
    }
    return false;
}

template <class T, class Abi> inline bool none_of(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = 0; i < N; ++i) {
        if (k[i]) {
            return false;
        }
    }
    return true;
}

template <class T, class Abi> inline bool some_of(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = 1; i < N; ++i) {
        if (k[i] != k[i - 1]) {
            return true;
        }
    }
    return false;
}

template <class T, class Abi> inline int popcount(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    int n = k[0];
    for (int i = 1; i < N; ++i) {
        n += k[i];
    }
    return n;
}

template <class T, class Abi> inline int find_first_set(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = 0; i < N; ++i) {
        if (k[i]) {
            return i;
        }
    }
    return -1;
}

template <class T, class Abi> inline int find_last_set(const mask<T, Abi> &k)
{
    constexpr int N = datapar_size_v<T, Abi>;
    for (int i = N - 1; i >= 0; --i) {
        if (k[i]) {
            return i;
        }
    }
    return -1;
}

#if !defined VC_COMMON_ALGORITHMS_H_
constexpr bool all_of(bool x) { return x; }
constexpr bool any_of(bool x) { return x; }
constexpr bool none_of(bool x) { return !x; }
constexpr bool some_of(bool) { return false; }
#endif
constexpr int popcount(bool x) { return x; }
constexpr int find_first_set(bool) { return 0; }
constexpr int find_last_set(bool) { return 0; }

// masked assignment [mask.where]
template <typename Mask, typename T> class where_expression
{
public:
    where_expression() = delete;
    where_expression(const where_expression &) = delete;
    where_expression(where_expression &&) = delete;
    where_expression &operator=(const where_expression &) = delete;
    where_expression &operator=(where_expression &&) = delete;
    Vc_INTRINSIC where_expression(const Mask kk, T &dd) : k(kk), d(dd) {}
    template <class U> Vc_INTRINSIC void operator=(U &&x)
    {
        using detail::masked_assign;
        masked_assign(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator+=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::plus>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator-=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::minus>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator*=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::multiplies>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator/=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::divides>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator%=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::modulus>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator&=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::bit_and>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator|=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::bit_or>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator^=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<std::bit_xor>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator<<=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<detail::shift_left>(k, d, std::forward<U>(x));
    }
    template <class U> Vc_INTRINSIC void operator>>=(U &&x)
    {
        using detail::masked_cassign;
        masked_cassign<detail::shift_right>(k, d, std::forward<U>(x));
    }
    Vc_INTRINSIC void operator++()
    {
        using detail::masked_unary;
        d = masked_unary<detail::increment>(k, d);
    }
    Vc_INTRINSIC void operator++(int)
    {
        using detail::masked_unary;
        d = masked_unary<detail::increment>(k, d);
    }
    Vc_INTRINSIC void operator--()
    {
        using detail::masked_unary;
        d = masked_unary<detail::decrement>(k, d);
    }
    Vc_INTRINSIC void operator--(int)
    {
        using detail::masked_unary;
        d = masked_unary<detail::decrement>(k, d);
    }
    Vc_INTRINSIC T operator-() const
    {
        using detail::masked_unary;
        return masked_unary<std::negate>(k, d);
    }
    Vc_INTRINSIC auto operator!() const { return k && !d; }

private:
    const Mask k;
    T &d;
};

template <class T0, class A0, class T1, class A1>
Vc_INTRINSIC where_expression<const mask<T1, A1> &, datapar<T1, A1>> where(
    const mask<T0, A0> &k, datapar<T1, A1> &d)
{
    return {k, d};
}
template <class T> Vc_INTRINSIC where_expression<bool, T> where(bool k, T &d)
{
    return {k, d};
}

// reductions [datapar.reductions]
template <class BinaryOperation = std::plus<>, class T, class Abi>
T reduce(const datapar<T, Abi> &, BinaryOperation = BinaryOperation());
template <class BinaryOperation = std::plus<>, class M, class T, class Abi>
T reduce(const where_expression<M, datapar<T, Abi>> &x, T init,
         BinaryOperation binary_op = BinaryOperation());

Vc_VERSIONED_NAMESPACE_END

#endif  // VC_DATAPAR_SYNOPSIS_H_

// vim: foldmethod=marker
