// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail.hpp"
#include "iota.hpp"

#include <cassert>
#include <array>
#include <algorithm>
#include <bit>

namespace irc::detail {

    template <
        typename _Ty, _Ty _Rfirst, _Ty _Rlast,
        typename _Sty
    >
    class bitset_base {
    protected:
        static_assert(std::is_same_v<std::decay_t<_Ty>, _Ty> && (std::is_integral_v<_Ty> || std::is_enum_v<_Ty>),
            "integral type (including bool) or enum is required");
        static_assert(std::is_same_v<_Sty, detail::_n_uint_t<sizeof(_Sty)>>,
            "storage item type (bucket) for irc::bitset_base<> must be one of std::uintN_t");

        constexpr static std::uintmax_t _Max = static_cast<std::uintmax_t>(_Rlast) - static_cast<std::uintmax_t>(_Rfirst);

        using _Tu = detail::_underlying_int_t<_Ty>;
        using _Ti = detail::_n_uint_t < std::max({ sizeof(_Tu), sizeof(std::size_t), sizeof(detail::_up_uint_t<_Max>) }) > ;
        static_assert(sizeof(_Ty) == sizeof(_Tu) && sizeof(_Ti) >= sizeof(_Tu));

        constexpr static _Tu _Ufirst = static_cast<_Tu>(_Rfirst);
        constexpr static _Tu _Ulast = static_cast<_Tu>(_Rlast);
        static_assert(_Ufirst <= _Ulast, "Invalid range");

        constexpr static _Ti _BktBytes = sizeof(_Sty);
        constexpr static _Ti _BktBits = _BktBytes * 8;

        static_assert(_Max < std::numeric_limits<_Ti>::max(), "Too big range");
        constexpr static _Ti _Range = static_cast<_Ti>(_Max) + 1u;
        constexpr static _Ti _NBkt = (_Range / _BktBits) + !!(_Range % _BktBits);
        static_assert(_NBkt >= 1);

        constexpr static _Ti _Inext = _BktBits;
        constexpr static _Ti _Imask = _Inext - 1;
        constexpr static _Ti _INmask = ~_Imask;
        constexpr static _Ti _IEnd = _NBkt * _BktBits;
        constexpr static _Ti _ILast = _Range - 1;
        constexpr static unsigned int _Ibits = std::bit_width(_Imask);

        constexpr static _Sty _Every = static_cast<_Sty>(-1);
        constexpr static _Sty _One = 1u;
        constexpr static _Sty _None = _Every ^ _One;
        constexpr static _Sty _Bmask = _Range % _BktBits ? static_cast<_Sty>(~(_Every << (_Range % _BktBits))) : _Every;

        using arr_t = std::array<_Sty, _NBkt>;
        using arr_it = typename arr_t::iterator;
        using arr_cit = typename arr_t::const_iterator;

        constexpr static auto _range = detail::_make_iota_wrapper<_Ty, _Tu, _Ti, _Ufirst, _Range>();

        template <typename _ItA>
        class _ref_base {
            constexpr _ref_base(_Sty i, _ItA p) : i{ i }, p{ p } {}
            constexpr explicit _ref_base(const _ref_base&) = default;
            constexpr _ref_base& operator =(const _ref_base&) = delete;

            constexpr static _ref_base at(_Ty v, _ItA a) noexcept {
                const _Tu u = static_cast<_Tu>(v);
                assert(u >= _Ufirst && u <= _Ulast);
                const _Ti pos = static_cast<_Ti>(u - _Ufirst);
                return at_pos(pos, a);
            }

            constexpr static _ref_base at_pos(_Ti v, _ItA a) noexcept {
                assert(v < _Range);
                if constexpr (_NBkt == 1) {
                    return _ref_base{ static_cast<_Sty>(_One << v), a };
                }
                else if constexpr (std::in_range<int>(_IEnd)) {
                    return _ref_base{ std::rotl(_One, static_cast<int>(v)), std::next(a, v >> _Ibits) };
                }
                else {
                    return _ref_base{ static_cast<_Sty>(_One << (v & _Imask)), std::next(a, v >> _Ibits) };
                }
            }

        protected:
            const _Sty i;
            const _ItA p;

        public:
            [[nodiscard]] constexpr operator bool() const noexcept {
                return test();
            }

            [[nodiscard]] constexpr bool operator ~() const noexcept {
                return !test();
            }

            constexpr bool test() const noexcept {
                return *p & i;
            }

            friend class bitset_base;
        };

        class _reference : public _ref_base<arr_it> {
            using _base = _ref_base<arr_it>;
            constexpr _reference(_Sty i, arr_it p) noexcept : _base{ i, p } {}
            constexpr explicit _reference(const _base& r) noexcept : _base{ r } {}

            constexpr static _reference at(_Ty v, arr_it a) noexcept {
                return _reference{ _base::at(v, a) };
            }

            constexpr static _reference at_pos(_Ti v, arr_it a) noexcept {
                return _reference{ _base::at_pos(v, a) };
            }

        public:
            constexpr _reference& operator =(bool x) noexcept {
                if (x) {
                    set();
                }
                else {
                    reset();
                }
                return *this;
            }

            constexpr _reference& operator =(const _reference& src) noexcept {
                return *this = src.operator bool();
            }

            constexpr void set() noexcept {
                *this->p |= this->i;
            }

            constexpr void reset() noexcept {
                *this->p &= ~this->i;
            }

            constexpr _reference& flip() noexcept {
                *this->p ^= this->i;
                return *this;
            }

            [[nodiscard]] constexpr bool test_and_set() noexcept {
                const _Sty v1 = *this->p;
                const _Sty v2 = v1 | this->i;
                *this->p = v2;
                return v1 == v2;
            }

            [[nodiscard]] constexpr bool test_and_reset() noexcept {
                const _Sty v1 = *this->p;
                const _Sty v2 = v1 & ~this->i;
                *this->p = v2;
                return v1 != v2;
            }

            friend class bitset_base;
        };

        arr_t a;

    public:
        constexpr static bool is_single_bucket = _NBkt == 1;
        using internal_array_type = arr_t;
        using reference = _reference;

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return _Range;
        }

        [[nodiscard]] constexpr std::size_t count() const noexcept {
            if constexpr (is_single_bucket) {
                return std::popcount(a[0]);
            } else {
                return std::transform_reduce(a.cbegin(), a.cend(), size_type{}, std::plus<>{},
                    [](_Sty b) { return std::popcount(b); });
            }
        }

        constexpr bitset_base() noexcept : a{} {}
        constexpr bitset_base(const bitset_base&) noexcept = default;

        constexpr explicit bitset_base(const internal_array_type& a) noexcept : a{a} {
            a.back() &= _Bmask;
        }

        constexpr explicit bitset_base(const _Sty* a, std::size_t n = _NBkt) noexcept : a{} {
            std::copy_n(a, std::min(_NBkt, n), this->a.begin());
            this->a.back() &= _Bmask;
        }

        [[nodiscard]] constexpr static std::size_t array_size() noexcept {
            return _NBkt;
        }

        [[nodiscard]] constexpr static _Sty back_mask() noexcept {
            return _Bmask;
        }

        [[nodiscard]] constexpr const internal_array_type& as_array() const noexcept {
            return a;
        }

        [[nodiscard]] constexpr internal_array_type& as_array() noexcept {
            // Important notice: do not modify bits in the array's last() item of internal array outside the back_mask().
            // Setting any bit to 1 outside the back_mask() leads to UB!
            return a;
        }

        [[nodiscard]] constexpr static auto range() noexcept {
            return _range;
        }

        [[nodiscard]] constexpr static _Ty first() noexcept {
            return _Rfirst;
        }

        [[nodiscard]] constexpr static _Ty last() noexcept {
            return _Rlast;
        }
    };
}

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
inline constexpr bool std::ranges::disable_sized_range<irc::detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>> = !irc::detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>::is_single_bucket;

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
struct std::hash<irc::detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>> {
    constexpr std::size_t operator ()(const irc::detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>& r) const noexcept {
        return std::hash<typename irc::detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>::internal_array_type>{}(r.as_array());
    }
};
