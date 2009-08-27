/*  This file is part of the Vc library.

    Copyright (C) 2009 Matthias Kretz <kretz@kde.org>

    Vc is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    Vc is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Vc.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "casts.h"
#include <cstdlib>

#ifndef VC_NO_BSF_LOOPS
# ifdef VC_NO_GATHER_TRICKS
#  define VC_NO_BSF_LOOPS
# elif !defined(__x86_64__) // 32 bit x86 does not have enough registers
#  define VC_NO_BSF_LOOPS
# elif defined(_MSC_VER) // TODO: write inline asm version for MSVC
#  define VC_NO_BSF_LOOPS
# elif defined(__GNUC__) // gcc and icc work fine with the inline asm
# else
#  error "Check whether inline asm works, or define VC_NO_BSF_LOOPS"
# endif
#endif

namespace SSE
{
    template<> inline _M128I SortHelper<_M128I, 8>::sort(_M128I x)
    {
        _M128I lo, hi, y;
        // sort pairs
        y = _mm_shufflelo_epi16(_mm_shufflehi_epi16(x, _MM_SHUFFLE(2, 3, 0, 1)), _MM_SHUFFLE(2, 3, 0, 1));
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);
        x = _mm_blend_epi16(lo, hi, 0xaa);

        // merge left and right quads
        y = _mm_shufflelo_epi16(_mm_shufflehi_epi16(x, _MM_SHUFFLE(0, 1, 2, 3)), _MM_SHUFFLE(0, 1, 2, 3));
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);
        x = _mm_blend_epi16(lo, hi, 0xcc);
        y = _mm_srli_si128(x, 2);
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);
        x = _mm_blend_epi16(lo, _mm_slli_si128(hi, 2), 0xaa);

        // merge quads into octs
        y = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
        y = _mm_shufflelo_epi16(y, _MM_SHUFFLE(0, 1, 2, 3));
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);

        x = _mm_unpacklo_epi16(lo, hi);
        y = _mm_srli_si128(x, 8);
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);

        x = _mm_unpacklo_epi16(lo, hi);
        y = _mm_srli_si128(x, 8);
        lo = _mm_min_epi16(x, y);
        hi = _mm_max_epi16(x, y);

        return _mm_unpacklo_epi16(lo, hi);
    }
    template<> inline _M128I SortHelper<_M128I, 4>::sort(_M128I x)
    {
        // sort pairs
        _M128I y = _mm_shuffle_epi32(x, _MM_SHUFFLE(2, 3, 0, 1));
        _M128I l = _mm_min_epi32(x, y);
        _M128I h = _mm_max_epi32(x, y);
        x = _mm_unpacklo_epi32(l, h);
        y = _mm_unpackhi_epi32(h, l);

        // sort quads
        l = _mm_min_epi32(x, y);
        h = _mm_max_epi32(x, y);
        x = _mm_unpacklo_epi32(l, h);
        y = _mm_unpackhi_epi64(x, x);

        l = _mm_min_epi32(x, y);
        h = _mm_max_epi32(x, y);
        return _mm_unpacklo_epi32(l, h);
    }
    template<> inline _M128 SortHelper<_M128, 4>::sort(_M128 x)
    {
        _M128 y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 3, 0, 1));
        _M128 l = _mm_min_ps(x, y);
        _M128 h = _mm_max_ps(x, y);
        x = _mm_unpacklo_ps(l, h);
        y = _mm_unpackhi_ps(h, l);

        l = _mm_min_ps(x, y);
        h = _mm_max_ps(x, y);
        x = _mm_unpacklo_ps(l, h);
        y = _mm_movehl_ps(x, x);

        l = _mm_min_ps(x, y);
        h = _mm_max_ps(x, y);
        return _mm_unpacklo_ps(l, h);
//X         _M128 k = _mm_cmpgt_ps(x, y);
//X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(2, 2, 0, 0));
//X         x = _mm_blendv_ps(x, y, k);
//X         y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(1, 0, 3, 2));
//X         k = _mm_cmpgt_ps(x, y);
//X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(1, 0, 1, 0));
//X         x = _mm_blendv_ps(x, y, k);
//X         y = _mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 1, 2, 0));
//X         k = _mm_cmpgt_ps(x, y);
//X         k = _mm_shuffle_ps(k, k, _MM_SHUFFLE(0, 1, 1, 0));
//X         return _mm_blendv_ps(x, y, k);
    }
    template<> inline M256 SortHelper<M256, 8>::sort(M256 x)
    {
        typedef SortHelper<_M128, 4> H;

        _M128 a, b, l, h;
        a = H::sort(x[0]);
        b = H::sort(x[1]);

        // merge
        b = _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 1, 2, 3));
        l = _mm_min_ps(a, b);
        h = _mm_max_ps(a, b);

        a = _mm_unpacklo_ps(l, h);
        b = _mm_unpackhi_ps(l, h);
        l = _mm_min_ps(a, b);
        h = _mm_max_ps(a, b);

        a = _mm_unpacklo_ps(l, h);
        b = _mm_unpackhi_ps(l, h);
        l = _mm_min_ps(a, b);
        h = _mm_max_ps(a, b);

        x[0] = _mm_unpacklo_ps(l, h);
        x[1] = _mm_unpackhi_ps(l, h);
        return x;
    }
    template<> inline _M128D SortHelper<_M128D, 2>::sort(_M128D x)
    {
        const _M128D y = _mm_shuffle_pd(x, x, _MM_SHUFFLE2(0, 1));
        return _mm_unpacklo_pd(_mm_min_sd(x, y), _mm_max_sd(x, y));
    }

    struct GeneralHelpers
    {
        template<unsigned int scale, typename Base, typename IndexType, typename EntryType>
        static inline void maskedGatherStructHelper(
                Base &v, const IndexType &indexes, int mask, const EntryType *baseAddr
                ) {
#ifndef VC_NO_BSF_LOOPS
            if (sizeof(EntryType) == 2) {
                register unsigned long int bit;
                register unsigned long int index;
                register EntryType value;
                asm volatile(
                        "bsf %1,%0"            "\n\t"
                        "jz 1f"                "\n\t"
                        "0:"                   "\n\t"
                        "movzwl (%5,%0,2),%%ecx""\n\t"
                        "btr %0,%1"            "\n\t"
                        "imul %8,%%ecx"        "\n\t"
                        "movw (%6,%%rcx,1),%3" "\n\t"
                        "movw %3,(%7,%0,2)"    "\n\t"
                        "bsf %1,%0"            "\n\t"
                        "jnz 0b"               "\n\t"
                        "1:"                   "\n\t"
                        : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                        : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "n"(scale), "m"(indexes.d.v())
                        : "rcx"   );
            } else if (sizeof(EntryType) == 4) {
                if (sizeof(typename IndexType::EntryType) == 4) {
                    register unsigned long int bit;
                    register unsigned long int index;
                    register EntryType value;
                    asm volatile(
                            "bsf %1,%0"            "\n\t"
                            "jz 1f"                "\n\t"
                            "0:"                   "\n\t"
                            "mov (%5,%0,4),%%ecx"  "\n\t"
                            "btr %0,%1"            "\n\t"
                            "imul %8,%%ecx"        "\n\t"
                            "mov (%6,%%rcx,1),%3"  "\n\t"
                            "mov %3,(%7,%0,4)"     "\n\t"
                            "bsf %1,%0"            "\n\t"
                            "jnz 0b"               "\n\t"
                            "1:"                   "\n\t"
                            : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                            : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "n"(scale), "m"(indexes.d.v())
                            : "rcx"   );
                } else if (sizeof(typename IndexType::EntryType) == 2) {
                    register unsigned long int bit;
                    register unsigned long int index;
                    register EntryType value;
                    asm volatile(
                            "bsf %1,%0"            "\n\t"
                            "jz 1f"                "\n\t"
                            "0:"                   "\n\t"
                            "movzwl (%5,%0,2),%%ecx""\n\t"
                            "btr %0,%1"            "\n\t"
                            "imul %8,%%ecx"        "\n\t"
                            "mov (%6,%%rcx,1),%3"  "\n\t"
                            "mov %3,(%7,%0,4)"     "\n\t"
                            "bsf %1,%0"            "\n\t"
                            "jnz 0b"               "\n\t"
                            "1:"                   "\n\t"
                            : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                            : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "n"(scale), "m"(indexes.d.v())
                            : "rcx"   );
                } else {
                    abort();
                }
            } else if (sizeof(EntryType) == 8) {
                register unsigned long int bit;
                register unsigned long int index;
                register EntryType value;
                asm volatile(
                        "bsf %1,%0"            "\n\t"
                        "jz 1f"                "\n\t"
                        "0:"                   "\n\t"
                        "mov (%5,%0,4),%%ecx"  "\n\t"
                        "btr %0,%1"            "\n\t"
                        "imul %8,%%ecx"        "\n\t"
                        "mov (%6,%%rcx,1),%3"  "\n\t"
                        "mov %3,(%7,%0,8)"     "\n\t"
                        "bsf %1,%0"            "\n\t"
                        "jnz 0b"               "\n\t"
                        "1:"                   "\n\t"
                        : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                        : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "n"(scale), "m"(indexes.d.v())
                        : "rcx"   );
            } else {
                abort();
            }
#else
            typedef const char * Memory MAY_ALIAS;
            Memory const baseAddr2 = reinterpret_cast<Memory>(baseAddr);
# ifdef VC_NO_GATHER_TRICKS
            for (int i = 0; i < Base::Size; ++i) {
                if (mask & (1 << i)) {
                    v.d.m(i) = baseAddr2[scale * indexes.d.m(i)];
                }
            }
# else
            unrolled_loop16(i, 0, Base::Size,
                    EntryType entry = *reinterpret_cast<const EntryType *>(&baseAddr2[scale * indexes.d.m(i)]);
                    register EntryType tmp = v.d.m(i);
                    if (mask & (1 << i)) tmp = entry;
                    v.d.m(i) = tmp;
                    );
# endif
#endif
        }

        template<typename Base, typename IndexType, typename EntryType>
        static inline void maskedGatherHelper(
                Base &v, const IndexType &indexes, int mask, const EntryType *baseAddr
                ) {
#ifndef VC_NO_BSF_LOOPS
            if (sizeof(EntryType) == 2) {
                register unsigned long int bit;
                register unsigned long int index;
                register EntryType value;
                asm volatile(
                        "bsf %1,%0"            "\n\t"
                        "jz 1f"                "\n\t"
                        "0:"                   "\n\t"
                        "movzwl (%5,%0,2),%%ecx""\n\t"
                        "btr %0,%1"            "\n\t"
                        "movw (%6,%%rcx,2),%3" "\n\t"
                        "movw %3,(%7,%0,2)"    "\n\t"
                        "bsf %1,%0"            "\n\t"
                        "jnz 0b"               "\n\t"
                        "1:"                   "\n\t"
                        : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                        : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "m"(indexes.d.v())
                        : "rcx"   );
            } else if (sizeof(EntryType) == 4) {
                if (sizeof(typename IndexType::EntryType) == 4) {
                    register unsigned long int bit;
                    register unsigned long int index;
                    register EntryType value;
                    asm volatile(
                            "bsf %1,%0"            "\n\t"
                            "jz 1f"                "\n\t"
                            "0:"                   "\n\t"
                            "mov (%5,%0,4),%%ecx"  "\n\t"
                            "btr %0,%1"            "\n\t"
                            "mov (%6,%%rcx,4),%3"  "\n\t"
                            "mov %3,(%7,%0,4)"     "\n\t"
                            "bsf %1,%0"            "\n\t"
                            "jnz 0b"               "\n\t"
                            "1:"                   "\n\t"
                            : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                            : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "m"(indexes.d.v())
                            : "rcx"   );
                } else if (sizeof(typename IndexType::EntryType) == 2) {
                    register unsigned long int bit;
                    register unsigned long int index;
                    register EntryType value;
                    asm volatile(
                            "bsf %1,%0"            "\n\t"
                            "jz 1f"                "\n\t"
                            "0:"                   "\n\t"
                            "movzwl (%5,%0,2),%%ecx""\n\t"
                            "btr %0,%1"            "\n\t"
                            "mov (%6,%%rcx,4),%3"  "\n\t"
                            "mov %3,(%7,%0,4)"     "\n\t"
                            "bsf %1,%0"            "\n\t"
                            "jnz 0b"               "\n\t"
                            "1:"                   "\n\t"
                            : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                            : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "m"(indexes.d.v())
                            : "rcx"   );
                } else {
                    abort();
                }
            } else if (sizeof(EntryType) == 8) {
                register unsigned long int bit;
                register unsigned long int index;
                register EntryType value;
                asm volatile(
                        "bsf %1,%0"            "\n\t"
                        "jz 1f"                "\n\t"
                        "0:"                   "\n\t"
                        "mov (%5,%0,4),%%ecx"  "\n\t"
                        "btr %0,%1"            "\n\t"
                        "mov (%6,%%rcx,8),%3"  "\n\t"
                        "mov %3,(%7,%0,8)"     "\n\t"
                        "bsf %1,%0"            "\n\t"
                        "jnz 0b"               "\n\t"
                        "1:"                   "\n\t"
                        : "=&r"(bit), "+r"(mask), "=&r"(index), "=&r"(value), "+m"(v.d)
                        : "r"(&indexes.d.v()), "r"(baseAddr), "r"(&v.d), "m"(indexes.d.v())
                        : "rcx"   );
            } else {
                abort();
            }
#elif defined(VC_NO_GATHER_TRICKS)
            for (int i = 0; i < Base::Size; ++i) {
                if (mask & (1 << i)) {
                    v.d.m(i) = baseAddr[indexes.d.m(i)];
                }
            }
#else
            unrolled_loop16(i, 0, Base::Size,
                    EntryType entry = baseAddr[indexes.d.m(i)];
                    register EntryType tmp = v.d.m(i);
                    if (mask & (1 << i)) tmp = entry;
                    v.d.m(i) = tmp;
                    );
#endif
        }

        template<unsigned int bitMask, typename AliasingT, typename EntryType>
        static inline void maskedScatterHelper(
                const AliasingT &vEntry, const int mask, EntryType &value
                ) {
#ifdef _MSC_VER
            register EntryType t;
            __asm {
                    test mask, bitMask
                    mov t, value
                    cmovne t, vEntry
                    mov value, t
            }
            return;
#elif defined(__GNUC__)
#ifndef __x86_64__ // on 32 bit use the non-asm-code below for sizeof(EntryType) > 4
            if (sizeof(EntryType) <= 4) {
#endif
            register EntryType t;
            asm(
                    "test %4,%2\n\t"
                    "mov %3,%1\n\t"
                    "cmovne %5,%1\n\t"
                    "mov %1,%0"
                    : "=m"(value), "=&r"(t)
                    : "r"(mask), "m"(value), "n"(bitMask), "m"(vEntry)
               );
            return;
#ifndef __x86_64__
            }
#endif
#endif // __GNUC__
            if (mask & bitMask) {
                value = vEntry;
            }
        }
    };

    ////////////////////////////////////////////////////////
    // Array gathers
    template<typename T> inline void GatherHelper<T>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        for_all_vector_entries(i,
                v.d.m(i) = baseAddr[indexes.d.m(i)];
                );
    }
    template<> inline void GatherHelper<double>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_pd(baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<float>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_ps(
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<float8>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v()[1] = _mm_set_ps(
                baseAddr[indexes.d.m(7)], baseAddr[indexes.d.m(6)],
                baseAddr[indexes.d.m(5)], baseAddr[indexes.d.m(4)]);
        v.d.v()[0] = _mm_set_ps(
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<int>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<unsigned int>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<short>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)], baseAddr[indexes.d.m(6)],
                baseAddr[indexes.d.m(5)], baseAddr[indexes.d.m(4)],
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }
    template<> inline void GatherHelper<unsigned short>::gather(
            Base &v, const IndexType &indexes, const EntryType *baseAddr)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)], baseAddr[indexes.d.m(6)],
                baseAddr[indexes.d.m(5)], baseAddr[indexes.d.m(4)],
                baseAddr[indexes.d.m(3)], baseAddr[indexes.d.m(2)],
                baseAddr[indexes.d.m(1)], baseAddr[indexes.d.m(0)]);
    }

    ////////////////////////////////////////////////////////
    // Struct gathers
    template<typename T> template<typename S1> inline void GatherHelper<T>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        for_all_vector_entries(i,
                v.d.m(i) = baseAddr[indexes.d.m(i)].*(member1);
                );
    }
    template<> template<typename S1> inline void GatherHelper<double>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_pd(baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<float>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_ps(
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<float8>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v()[1] = _mm_set_ps(
                baseAddr[indexes.d.m(7)].*(member1), baseAddr[indexes.d.m(6)].*(member1),
                baseAddr[indexes.d.m(5)].*(member1), baseAddr[indexes.d.m(4)].*(member1));
        v.d.v()[0] = _mm_set_ps(
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<int>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<unsigned int>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<short>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)].*(member1), baseAddr[indexes.d.m(6)].*(member1),
                baseAddr[indexes.d.m(5)].*(member1), baseAddr[indexes.d.m(4)].*(member1),
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }
    template<> template<typename S1> inline void GatherHelper<unsigned short>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const EntryType S1::* member1)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)].*(member1), baseAddr[indexes.d.m(6)].*(member1),
                baseAddr[indexes.d.m(5)].*(member1), baseAddr[indexes.d.m(4)].*(member1),
                baseAddr[indexes.d.m(3)].*(member1), baseAddr[indexes.d.m(2)].*(member1),
                baseAddr[indexes.d.m(1)].*(member1), baseAddr[indexes.d.m(0)].*(member1));
    }

    ////////////////////////////////////////////////////////
    // Struct of Struct gathers
    template<typename T> template<typename S1, typename S2> inline void GatherHelper<T>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        for_all_vector_entries(i,
                v.d.m(i) = baseAddr[indexes.d.m(i)].*(member1).*(member2);
                );
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<double>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_pd(baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<float>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_ps(
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<float8>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v()[1] = _mm_set_ps(
                baseAddr[indexes.d.m(7)].*(member1).*(member2), baseAddr[indexes.d.m(6)].*(member1).*(member2),
                baseAddr[indexes.d.m(5)].*(member1).*(member2), baseAddr[indexes.d.m(4)].*(member1).*(member2));
        v.d.v()[0] = _mm_set_ps(
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<int>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<unsigned int>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_epi32(
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<short>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)].*(member1).*(member2), baseAddr[indexes.d.m(6)].*(member1).*(member2),
                baseAddr[indexes.d.m(5)].*(member1).*(member2), baseAddr[indexes.d.m(4)].*(member1).*(member2),
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }
    template<> template<typename S1, typename S2> inline void GatherHelper<unsigned short>::gather(
            Base &v, const IndexType &indexes, const S1 *baseAddr, const S2 S1::* member1, const EntryType S2::* member2)
    {
        v.d.v() = _mm_set_epi16(
                baseAddr[indexes.d.m(7)].*(member1).*(member2), baseAddr[indexes.d.m(6)].*(member1).*(member2),
                baseAddr[indexes.d.m(5)].*(member1).*(member2), baseAddr[indexes.d.m(4)].*(member1).*(member2),
                baseAddr[indexes.d.m(3)].*(member1).*(member2), baseAddr[indexes.d.m(2)].*(member1).*(member2),
                baseAddr[indexes.d.m(1)].*(member1).*(member2), baseAddr[indexes.d.m(0)].*(member1).*(member2));
    }

    ////////////////////////////////////////////////////////
    // Scatters
    //
    // There is no equivalent to the set intrinsics. Therefore the vector entries are copied in
    // memory instead from the xmm register directly.
    //
    // TODO: With SSE 4.1 the extract intrinsics might be an interesting option, though.
    //
    template<typename T> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, EntryType *baseAddr) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)] = v.d.m(i);
                );
    }
    template<> inline void VectorHelperSize<short>::scatter(
            const Base &v, const IndexType &indexes, EntryType *baseAddr) {
        // TODO: verify that using extract is really faster
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)] = _mm_extract_epi16(v.d.v(), i);
                );
    }

    template<typename T> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, int mask, EntryType *baseAddr) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)]);
                );
    }

    template<typename T> template<typename S1> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, S1 *baseAddr, EntryType S1::* member1) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)].*(member1) = v.d.m(i);
                );
    }

    template<typename T> template<typename S1> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, int mask, S1 *baseAddr, EntryType S1::* member1) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)].*(member1));
                );
    }

    template<typename T> template<typename S1, typename S2> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, S1 *baseAddr, S2 S1::* member1, EntryType S2::* member2) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)].*(member1).*(member2) = v.d.m(i);
                );
    }

    template<typename T> template<typename S1, typename S2> inline void VectorHelperSize<T>::scatter(
            const Base &v, const IndexType &indexes, int mask, S1 *baseAddr, S2 S1::* member1,
            EntryType S2::* member2) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)].*(member1).*(member2));
                );
    }

    inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes, EntryType *baseAddr) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)] = v.d.m(i);
                );
    }

    inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes, int mask, EntryType *baseAddr) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)]);
                );
    }

    template<typename S1> inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes,
            S1 *baseAddr, EntryType S1::* member1) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)].*(member1) = v.d.m(i);
                );
    }

    template<typename S1> inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes, int mask,
            S1 *baseAddr, EntryType S1::* member1) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)].*(member1));
                );
    }

    template<typename S1, typename S2> inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes,
            S1 *baseAddr, S2 S1::* member1, EntryType S2::* member2) {
        for_all_vector_entries(i,
                baseAddr[indexes.d.m(i)].*(member1).*(member2) = v.d.m(i);
                );
    }

    template<typename S1, typename S2> inline void VectorHelperSize<float8>::scatter(const Base &v, const IndexType &indexes, int mask,
            S1 *baseAddr, S2 S1::* member1, EntryType S2::* member2) {
        for_all_vector_entries(i,
                GeneralHelpers::maskedScatterHelper<1 << i * Shift>(v.d.m(i), mask, baseAddr[indexes.d.m(i)].*(member1).*(member2));
                );
    }

    // can be used to multiply with a constant. For some special constants it doesn't need an extra
    // vector but can use a shift instead, basically encoding the factor in the instruction.
    template<typename IndexType, unsigned int constant> inline IndexType mulConst(const IndexType &x) {
        typedef VectorHelper<typename IndexType::EntryType> H;
        switch (constant) {
            case    0: return H::zero();
            case    1: return x;
            case    2: return H::slli(x.data(),  1);
            case    4: return H::slli(x.data(),  2);
            case    8: return H::slli(x.data(),  3);
            case   16: return H::slli(x.data(),  4);
            case   32: return H::slli(x.data(),  5);
            case   64: return H::slli(x.data(),  6);
            case  128: return H::slli(x.data(),  7);
            case  256: return H::slli(x.data(),  8);
            case  512: return H::slli(x.data(),  9);
            case 1024: return H::slli(x.data(), 10);
            case 2048: return H::slli(x.data(), 11);
        }
#ifndef __SSE4_1__
        // without SSE 4.1 int multiplication is not so nice
        if (sizeof(typename IndexType::EntryType) == 4) {
            switch (constant) {
                case    3: return H::add(        x.data()    , H::slli(x.data(),  1));
                case    5: return H::add(        x.data()    , H::slli(x.data(),  2));
                case    9: return H::add(        x.data()    , H::slli(x.data(),  3));
                case   17: return H::add(        x.data()    , H::slli(x.data(),  4));
                case   33: return H::add(        x.data()    , H::slli(x.data(),  5));
                case   65: return H::add(        x.data()    , H::slli(x.data(),  6));
                case  129: return H::add(        x.data()    , H::slli(x.data(),  7));
                case  257: return H::add(        x.data()    , H::slli(x.data(),  8));
                case  513: return H::add(        x.data()    , H::slli(x.data(),  9));
                case 1025: return H::add(        x.data()    , H::slli(x.data(), 10));
                case 2049: return H::add(        x.data()    , H::slli(x.data(), 11));
                case    6: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  2));
                case   10: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  3));
                case   18: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  4));
                case   34: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  5));
                case   66: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  6));
                case  130: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  7));
                case  258: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  8));
                case  514: return H::add(H::slli(x.data(), 1), H::slli(x.data(),  9));
                case 1026: return H::add(H::slli(x.data(), 1), H::slli(x.data(), 10));
                case 2050: return H::add(H::slli(x.data(), 1), H::slli(x.data(), 11));
                case   12: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  3));
                case   20: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  4));
                case   36: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  5));
                case   68: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  6));
                case  132: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  7));
                case  260: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  8));
                case  516: return H::add(H::slli(x.data(), 2), H::slli(x.data(),  9));
                case 1028: return H::add(H::slli(x.data(), 2), H::slli(x.data(), 10));
                case 2052: return H::add(H::slli(x.data(), 2), H::slli(x.data(), 11));
                case   24: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  4));
                case   40: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  5));
                case   72: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  6));
                case  136: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  7));
                case  264: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  8));
                case  520: return H::add(H::slli(x.data(), 3), H::slli(x.data(),  9));
                case 1032: return H::add(H::slli(x.data(), 3), H::slli(x.data(), 10));
                case 2056: return H::add(H::slli(x.data(), 3), H::slli(x.data(), 11));
                case   48: return H::add(H::slli(x.data(), 4), H::slli(x.data(),  5));
                case   80: return H::add(H::slli(x.data(), 4), H::slli(x.data(),  6));
                case  144: return H::add(H::slli(x.data(), 4), H::slli(x.data(),  7));
                case  272: return H::add(H::slli(x.data(), 4), H::slli(x.data(),  8));
                case  528: return H::add(H::slli(x.data(), 4), H::slli(x.data(),  9));
                case 1040: return H::add(H::slli(x.data(), 4), H::slli(x.data(), 10));
                case 2064: return H::add(H::slli(x.data(), 4), H::slli(x.data(), 11));
                case   96: return H::add(H::slli(x.data(), 5), H::slli(x.data(),  6));
                case  160: return H::add(H::slli(x.data(), 5), H::slli(x.data(),  7));
                case  288: return H::add(H::slli(x.data(), 5), H::slli(x.data(),  8));
                case  544: return H::add(H::slli(x.data(), 5), H::slli(x.data(),  9));
                case 1056: return H::add(H::slli(x.data(), 5), H::slli(x.data(), 10));
                case 2080: return H::add(H::slli(x.data(), 5), H::slli(x.data(), 11));
                case  192: return H::add(H::slli(x.data(), 6), H::slli(x.data(),  7));
                case  320: return H::add(H::slli(x.data(), 6), H::slli(x.data(),  8));
                case  576: return H::add(H::slli(x.data(), 6), H::slli(x.data(),  9));
                case 1088: return H::add(H::slli(x.data(), 6), H::slli(x.data(), 10));
                case 2112: return H::add(H::slli(x.data(), 6), H::slli(x.data(), 11));
                case  384: return H::add(H::slli(x.data(), 7), H::slli(x.data(),  8));
                case  640: return H::add(H::slli(x.data(), 7), H::slli(x.data(),  9));
                case 1152: return H::add(H::slli(x.data(), 7), H::slli(x.data(), 10));
                case 2176: return H::add(H::slli(x.data(), 7), H::slli(x.data(), 11));
                case  768: return H::add(H::slli(x.data(), 8), H::slli(x.data(),  9));
                case 1280: return H::add(H::slli(x.data(), 8), H::slli(x.data(), 10));
                case 2304: return H::add(H::slli(x.data(), 8), H::slli(x.data(), 11));
                case 1536: return H::add(H::slli(x.data(), 9), H::slli(x.data(), 10));
                case 2560: return H::add(H::slli(x.data(), 9), H::slli(x.data(), 11));
                case 3072: return H::add(H::slli(x.data(),10), H::slli(x.data(), 11));
            }
        }
#endif
        return H::mul(x.data(), H::set(constant));
    }
} // namespace SSE
