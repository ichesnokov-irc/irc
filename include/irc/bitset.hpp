// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail/bitset_base.hpp"
#include "auto_range.hpp"
#include "enum_range.hpp"
#include "forward.hpp"

#include <string>
#include <string_view>
#include <iosfwd>
#include <locale>

namespace irc {

    template <
        typename _Ty, _Ty _Rfirst, _Ty _Rlast,
        typename _Sty = detail::_fast_sty_for_range_t<_Ty, _Rfirst, _Rlast>
    >
    class bitset : protected detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty> {
        using _Base = detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>;

    public:
        using
            _Base::_Base,
            _Base::operator =,
            _Base::is_single_bucket,
            _Base::internal_array_type,
            _Base::reference,
            _Base::size,
            _Base::count,
            _Base::array_size,
            _Base::back_mask,
            _Base::as_array,
            _Base::first,
            _Base::last,
            _Base::range;

        constexpr explicit bitset(unsigned long long val) noexcept : _Base{} {
            if (sizeof(_Sty) >= sizeof(unsigned long long) || std::bit_width(val) <= _BktBits) {
                _Sty v = static_cast<_Sty>(val);
                if constexpr (_Bmask != _Every && is_single_bucket) {
                    v &= _Bmask;
                }
                a[0] = v;
            }
            else {
                arr_it it = a.begin();
                while (val != 0 && it != a.end()) {
                    *it++ = static_cast<_Sty>(val);
                    val >>= _BktBits;
                }
                a.back() &= _Bmask;
            }
        }

        template <typename _CharT>
        constexpr explicit bitset(const _CharT* s, typename std::basic_string<_CharT>::std::size_t n = std::size_t(-1),
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : bitset(
                n == std::basic_string<_CharT>::npos
                ? std::basic_string_view<_CharT>(s)
                : std::basic_string_view<_CharT>(s, n), 0, n, zero, one
            )
        {}

        template <typename _CharT, typename _Traits, typename _Alloc>
        constexpr explicit bitset(const std::basic_string<_CharT, _Traits, _Alloc>& s,
            typename std::basic_string<_CharT, _Traits, _Alloc>::std::size_t pos = 0,
            typename std::basic_string<_CharT, _Traits, _Alloc>::std::size_t n = std::basic_string<_CharT, _Traits, _Alloc>::npos,
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : bitset(
                static_cast<std::basic_string_view<_CharT, _Traits>>(s), pos, n, zero, one
            )
        {}

        template <typename _CharT, typename _Traits>
        constexpr explicit bitset(const std::basic_string_view<_CharT, _Traits>& s,
            typename std::basic_string_view<_CharT, _Traits>::std::size_t pos = 0,
            typename std::basic_string_view<_CharT, _Traits>::std::size_t n = std::size_t(-1),
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : _Base{} {
            const std::size_t end = std::min({ n, _Range, s.length() - pos });
            const auto ss = s.substr(pos, end);
            auto&& it = ss.crbegin();
            for (std::size_t i = 0; i != end; ++i) {
                const _CharT c = *it++;
                if (!(_Traits::eq(c, zero) || _Traits::eq(c, one))) {
                    throw std::invalid_argument("only zero or one is expected in input string");
                }
                if (_Traits::eq(c, one)) {
                    _ref_pos(i).set();
                }
            }
        }

        template <typename _CharT = char, typename _Traits = std::char_traits<_CharT>, typename _Alloc = std::allocator<_CharT>>
        [[nodiscard]] constexpr std::basic_string<_CharT, _Traits, _Alloc> to_string(_CharT zero = _CharT('0'), _CharT one = _CharT('1')) const {
            const std::array<_CharT, 2> zo = { zero, one };
            std::basic_string<_CharT, _Traits, _Alloc> s;
            s.reserve(max_size());
            rfor_every([&s, zo](_Ty, bool b) noexcept {
                s.push_back(zo[b]);
                });
            return s;
        }

        [[nodiscard]] constexpr unsigned long to_ulong() const {
            using ulong = unsigned long;
            if (width() > sizeof(ulong) * 8) {
                throw std::overflow_error("value can't fit in ulong");
            }
            if constexpr (sizeof(_Sty) >= sizeof(ulong)) {
                return static_cast<ulong>(a[0]);
            }
            else {
                ulong r = 0;
                std::size_t i = std::min(a.size(), sizeof(ulong) / sizeof(_Sty));
                while (i-- != 0) {
                    r <<= _BktBits;
                    r |= a[i];
                }
                return r;
            }
        }

        [[nodiscard]] constexpr unsigned long long to_ullong() const {
            using ullong = unsigned long long;
            if (width() > sizeof(ullong) * 8) {
                throw std::overflow_error("value can't fit in ullong");
            }
            if constexpr (sizeof(_Sty) >= sizeof(ullong)) {
                return static_cast<ullong>(a[0]);
            }
            else {
                ullong r = 0;
                std::size_t i = std::min(a.size(), sizeof(ullong) / sizeof(_Sty));
                while (i-- != 0) {
                    r <<= _BktBits;
                    r |= a[i];
                }
                return r;
            }
        }

        [[nodiscard]] constexpr bool operator [](_Ty pos) const noexcept {
            assert(_in_range(pos));
            return _ref(pos);
        }

        [[nodiscard]] constexpr reference operator [](_Ty pos) noexcept {
            assert(_in_range(pos));
            return _ref(pos);
        }

        [[nodiscard]] constexpr bool test(_Ty pos) const {
            if (!_in_range(pos)) {
                throw std::out_of_range("position outside the range");
            }
            return _ref(pos);
        }

        [[nodiscard]] constexpr bool all() const noexcept {
            return a.back() == _Bmask &&
                std::all_of(a.cbegin(), std::prev(a.cend()), [](_Sty b) { return b == _Every; });
        }

        [[nodiscard]] constexpr bool any() const noexcept {
            return std::any_of(a.cbegin(), a.cend(), [](_Sty b) { return b; });
        }

        [[nodiscard]] constexpr bool none() const noexcept {
            return std::none_of(a.cbegin(), a.cend(), [](_Sty b) { return b; });
        }

        constexpr bitset& operator &=(const bitset& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_and{});
            return *this;
        }

        constexpr bitset& operator |=(const bitset& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_or{});
            return *this;
        }

        constexpr bitset& operator ^=(const bitset& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_xor{});
            return *this;
        }

        constexpr bitset& operator -=(const bitset& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), [](_Sty b1, _Sty b2) { return b1 & ~b2; });
            return *this;
        }

        constexpr bitset operator ~() const noexcept {
            bitset result;
            std::transform(a.cbegin(), std::prev(a.cend()), result.a.begin(), [](_Sty b) { return ~b; });
            result.a.back() = ~a.back() & _Bmask;
            return result;
        }

        constexpr bitset& operator <<=(std::size_t pos) noexcept {
            if (pos >= _Range) {
                clear();
            }
            else if (pos != 0) [[likely]] {
                arr_it dst = _shl(pos, a.end());
                std::fill(a.begin(), dst, 0);
                a.back() &= _Bmask;
            }
            return *this;
        }

        constexpr bitset operator <<(std::size_t pos) const noexcept {
            bitset res;
            if (pos < _Range) {
                _shl(pos, res.a.end());
                res.a.back() &= _Bmask;
            }
            return res;
        }

        constexpr bitset& operator >>=(std::size_t pos) noexcept {
            if (pos >= _Range) {
                clear();
            }
            else if (pos != 0) [[likely]] {
                arr_it dst = _shr(pos, a.begin());
                std::fill(dst, a.end(), 0);
            }
            return *this;
        }

        constexpr bitset operator >>(std::size_t pos) const noexcept {
            bitset res;
            if (pos < _Range) {
                _shr(pos, res.a.begin());
            }
            return res;
        }

        constexpr bitset& set() noexcept {
            std::fill(a.begin(), std::prev(a.end()), _Every);
            a.back() = _Bmask;
            return *this;
        }

        constexpr bitset& set(_Ty pos, bool value = true) {
            if (!_in_range(pos)) {
                throw std::out_of_range("position outside the range");
            }
            _ref(pos) = value;
            return *this;
        }

        constexpr bitset& reset() noexcept {
            clear();
            return *this;
        }

        constexpr bitset& reset(_Ty pos) {
            if (!_in_range(pos)) {
                throw std::out_of_range("position outside the range");
            }
            _ref(pos).reset();
            return *this;
        }

        constexpr bitset& flip() noexcept {
            std::transform(a.cbegin(), std::prev(a.cend()), a.begin(), [](_Sty b) { return ~b; });
            a.back() ^= _Bmask;
            return *this;
        }

        constexpr bitset& flip(_Ty pos) {
            if (!_in_range(pos)) {
                throw std::out_of_range("position outside the range");
            }
            _ref(pos).flip();
            return *this;
        }

    public:
        template <bool _01, typename _UnaryPred>
        constexpr std::size_t flip_if(_UnaryPred&& pred) noexcept(std::is_nothrow_invocable_v<_UnaryPred, _Ty>) {
            std::size_t n = 0;
            _Tu ub = _Ufirst;
            const auto _Iter = [pred = std::forward<_UnaryPred>(pred), &n, &ub](_Sty b) noexcept(std::is_nothrow_invocable_v<_UnaryPred, _Ty>) {
                _Sty m = _Every;
                while (b & m) {
                    const unsigned int c = std::countr_zero(static_cast<_Sty>(b & m));
                    m = _Every << c;
                    _Tu u = ub + c;
                    _Sty i = _One << c;
                    do {
                        if (pred(static_cast<_Ty>(u++))) {
                            b ^= i, ++n;
                        }
                        m <<= 1u, i <<= 1u;
                    } while (b & i);
                }
                ub += _Inext;
                return b;
            };
            if constexpr (_01) {
                std::transform(a.cbegin(), a.cend(), a.begin(), _Iter);
            }
            else {
                std::transform(a.cbegin(), std::prev(a.cend()), a.begin(), [&_Iter](_Sty b) { return ~_Iter(~b); });
                a.back() = ~_Iter(~a.back() & _Bmask) & _Bmask;
            }
            return n;
        }

        template <typename _BinaryPred>
        constexpr std::size_t flip_if(_BinaryPred&& pred) noexcept(std::is_nothrow_invocable_v<_BinaryPred, _Ty, const_reference>) {
            std::size_t n = 0;
            arr_it p = a.begin();
            _Sty i = _One;
            _Tu u = _Ufirst;
            do {
                if (pred(static_cast<_Ty>(u), const_reference{ i, p })) {
                    *p ^= i, ++n;
                }
                if (u++ == _Ulast) {
                    break;
                }
                i = std::rotl(i, 1u);
                p += i & _One;
            } while (1);
            return n;
        }

        constexpr void flip(_Ty from, _Ty to) noexcept {
            const _Tu u1 = static_cast<_Tu>(from), u2 = static_cast<_Tu>(to);
            assert(u1 >= _Ufirst && u2 <= _Ulast && u1 <= u2);
            const reference r1 = _ref(static_cast<_Ty>(u1));
            const reference r2 = _ref(static_cast<_Ty>(u2));
            const _Sty m1 = r1.i - 1u;
            const _Sty m2 = (r2.i - 1u) | r2.i;
            if (r1.p == r2.p) {
                *r1.p ^= m2 ^ m1;
            }
            else {
                *r1.p ^= ~m1;
                *r2.p ^= m2;
                std::transform(std::next(r1.p), r2.p, std::next(r1.p), [](_Sty b) { return ~b; });
            }
            a.back() &= _Bmask;
        }

        template <bool _01, typename _UnaryFn>
        constexpr void for_every(_UnaryFn&& fn) const noexcept(std::is_nothrow_invocable_v<_UnaryFn, _Ty>) {
            _Tu ub = _Ufirst;
            const auto _Iter = [fn = std::forward<_UnaryFn>(fn), &ub](_Sty b) noexcept(std::is_nothrow_invocable_v<_UnaryFn, _Ty>) {
                _Tu u = ub;
                while (b) {
                    unsigned int c = std::countr_zero(b);
                    u += c, b >>= c;
                    c = std::countr_one(b);
                    b >>= c - 1u, b >>= 1u;
                    do {
                        fn(static_cast<_Ty>(u++));
                    } while (--c);
                }
                ub += _Inext;
            };
            if constexpr (_01) {
                std::for_each(a.cbegin(), a.cend(), _Iter);
            }
            else {
                std::for_each(a.cbegin(), std::prev(a.cend()), [&_Iter](_Sty b) { _Iter(~b); });
                _Iter(~a.back() & _Bmask);
            }
        }

        template <typename _BinaryFn>
        constexpr void for_every(_BinaryFn&& fn) const noexcept(std::is_nothrow_invocable_v<_BinaryFn, _Ty, const_reference>) {
            arr_cit p = a.cbegin();
            _Sty i = _One;
            _Tu u = _Ufirst;
            do {
                fn(static_cast<_Ty>(u), const_reference{ i, p });
                if (u++ == _Ulast) {
                    break;
                }
                i = std::rotl(i, 1u);
                p += i & _One;
            } while (1);
        }

        template <bool _01, typename _UnaryFn>
        constexpr void rfor_every(_UnaryFn&& fn) const noexcept(std::is_nothrow_invocable_v<_UnaryFn, _Ty>) {
            _Tu ub = static_cast<_Tu>(_IEnd + _Ufirst);
            const auto _Riter = [fn = std::forward<_UnaryFn>(fn), &ub](_Sty b) noexcept(std::is_nothrow_invocable_v<_UnaryFn, _Ty>) {
                _Tu u = ub;
                while (b) {
                    unsigned int c = std::countl_zero(b);
                    u -= c, b <<= c;
                    c = std::countl_one(b);
                    b <<= c - 1u, b <<= 1u;
                    do {
                        fn(static_cast<_Ty>(--u));
                    } while (--c);
                }
                ub -= _Inext;
            };
            if constexpr (_01) {
                std::for_each(a.crbegin(), a.crend(), _Riter);
            }
            else {
                _Riter(~a.back() & _Bmask);
                std::for_each(std::next(a.crbegin()), a.crend(), [&_Riter](_Sty b) { _Riter(~b); });
            }
        }

        template <typename _BinaryFn>
        constexpr void rfor_every(_BinaryFn&& fn) const noexcept(std::is_nothrow_invocable_v<_BinaryFn, _Ty, const_reference>) {
            arr_cit p = std::prev(a.cend());
            _Sty i = std::bit_floor(_Bmask);
            _Tu u = _Ulast;
            do {
                fn(static_cast<_Ty>(u), const_reference{ i, p });
                if (u-- == _Ufirst) {
                    break;
                }
                p -= i & _One;
                i = std::rotr(i, 1u);
            } while (1);
        }

        template <bool _01>
        constexpr void fill(_Ty from, _Ty to) noexcept {
            _Tu u1 = static_cast<_Tu>(from), u2 = static_cast<_Tu>(to);
            if constexpr (_01) {
                assert(u1 >= _Ufirst && u2 <= _Ulast);
            }
            else {
                u1 = std::max(u1, _Ufirst);
            }
            assert(u1 <= u2);
            const reference r1 = _ref(static_cast<_Ty>(u1));
            if (u2 >= _Ulast) {
                const _Sty m = r1.i - 1u;
                if constexpr (_01) {
                    *r1.p |= ~m;
                    std::fill(std::next(r1.p), a.end(), _Every);
                }
                else {
                    *r1.p &= m;
                    std::fill(std::next(r1.p), a.end(), _Sty{});
                }
            }
            else {
                const reference r2 = _ref(static_cast<_Ty>(u2));
                const _Sty m1 = r1.i - 1u;
                const _Sty m2 = (r2.i - 1u) | r2.i;
                if constexpr (_01) {
                    if (r1.p == r2.p) {
                        *r1.p |= m2 ^ m1;
                    }
                    else {
                        *r1.p |= ~m1;
                        *r2.p |= m2;
                        std::fill(std::next(r1.p), r2.p, _Every);
                    }
                }
                else {
                    if (r1.p == r2.p) {
                        *r1.p &= ~(m2 ^ m1);
                    }
                    else {
                        *r1.p &= m1;
                        *r2.p &= ~m2;
                        std::fill(std::next(r1.p), r2.p, _Sty{});
                    }
                }
            }
            if constexpr (_01) {
                a.back() &= _Bmask;
            }
        }

        template <unsigned int _Radix = 10, typename _CharT = char, typename _Traits = std::char_traits<_CharT>, typename _Alloc = std::allocator<_CharT>>
        [[nodiscard]] constexpr std::basic_string<_CharT, _Traits, _Alloc> to_ustring(_CharT left = _CharT('['), _CharT right = _CharT(']'), _CharT sep = _CharT(',')) const {
            std::basic_string<_CharT, _Traits, _Alloc> s;
            const std::size_t capacity = 2 + size() * (1 + std::max(detail::_ndigits<_Radix>(_Ufirst), detail::_ndigits<_Radix>(_Ulast)));
            s.reserve(capacity);
            s += left;
            std::size_t i = 0;
            for_every<1>([&s, &i, sep](_Ty v) noexcept {
                if (i++) {
                    s += sep;
                }
                _Tu u = static_cast<_Tu>(v);
                if (u < 0) {
                    s += _CharT('-');
                    u = static_cast<_Tu>(-u);
                }
                const auto n = s.size();
                do {
                    const auto d = u % _Radix;
                    s += static_cast<_CharT>((d <= 9 ? '0' : ('a' - 10)) + d);
                    u /= _Radix;
                } while (u);
                std::reverse(std::next(s.begin(), n), s.end());
                });
            s += right;
            return s;
        }

        [[nodiscard]] constexpr std::size_t width() const noexcept {
            _Ti w = _IEnd;
            for (auto it = a.crbegin(); it != a.crend(); ++it) {
                if (*it) {
                    w -= std::countl_zero(*it);
                    break;
                }
                else {
                    w -= _BktBits;
                }
            }
            return w;
        }

        [[nodiscard]] constexpr bool intersects(const bitset& other) const noexcept {
            for (arr_cit it1 = a.cbegin(), it2 = other.a.cbegin(); it1 != a.cend(); ++it1, ++it2) {
                if (*it1 & *it2) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] constexpr bool is_subset_of(const bitset& superset) const noexcept {
            for (arr_cit it1 = a.cbegin(), it2 = superset.a.cbegin(); it1 != a.cend(); ++it1, ++it2) {
                if ((*it1 & *it2) != *it1) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] constexpr bool is_proper_subset_of(const bitset& superset) const noexcept {
            bool proper = false;
            for (arr_cit it1 = a.cbegin(), it2 = superset.a.cbegin(); it1 != a.cend(); ++it1, ++it2) {
                if ((*it1 & *it2) != *it1) {
                    return false;
                }
                proper |= *it1 != *it2;
            }
            return proper;
        }

    private:
        constexpr static bool _in_range(const _Ty& v) noexcept {
            const _Tu u = static_cast<_Tu>(v);
            return u >= _Ufirst && u <= _Ulast;
        }

        constexpr arr_it _shl(std::size_t pos, arr_it dst) const noexcept {
            assert(pos < _Range);
            if constexpr (is_single_bucket) {
                *(--dst) = (pos >= _BktBits ? 0 : a[0] << pos);
            }
            else {
                const std::size_t shl_by = pos >> _Ibits;
                const std::size_t shl_bi = pos & _Imask;
                arr_cit src = std::prev(a.cend(), shl_by);
                if (shl_bi) {
                    const std::size_t br = _BktBits - shl_bi;
                    std::size_t n = std::distance(a.cbegin(), src);
                    while (--n) {
                        --dst, --src;
                        *dst = (*src << shl_bi) | (*std::prev(src) >> br);
                    }
                    *(--dst) = *std::prev(src) << shl_bi;
                }
                else {
                    dst = std::copy_backward(a.cbegin(), src, dst);
                }
            }
            return dst;
        }

        constexpr arr_it _shr(std::size_t pos, arr_it dst) const noexcept {
            assert(pos < _Range);
            if constexpr (is_single_bucket) {
                *dst++ = (pos >= _BktBits ? 0 : a[0] >> pos);
            }
            else {
                const std::size_t shr_by = pos >> _Ibits;
                const std::size_t shr_bi = pos & _Imask;
                arr_cit src = std::next(a.cbegin(), shr_by);
                if (shr_bi) {
                    const std::size_t bl = _BktBits - shr_bi;
                    std::size_t n = std::distance(src, a.cend());
                    while (--n) {
                        *dst = (*src >> shr_bi) | (*std::next(src) << bl);
                        ++dst, ++src;
                    }
                    *dst++ = *src >> shr_bi;
                }
                else {
                    dst = std::copy(src, a.cend(), dst);
                }
            }
            return dst;
        }

    public:
        friend constexpr bool operator ==(const bitset& lr, const bitset& rr) noexcept = default;
        friend constexpr std::strong_ordering operator <=>(const bitset& lr, const bitset& rr) noexcept = default;

        friend constexpr void swap(bitset& lr, bitset& rr) noexcept {
            lr.a.swap(rr.a);
        }

        friend constexpr bitset operator &(const bitset& lr, const bitset& rr) noexcept {
            bitset res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_and{});
            return res;
        }

        friend constexpr bitset operator |(const bitset& lr, const bitset& rr) noexcept {
            bitset res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_or{});
            return res;
        }

        friend constexpr bitset operator ^(const bitset& lr, const bitset& rr) noexcept {
            bitset res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_xor{});
            return res;
        }

        friend constexpr bitset operator -(const bitset& lr, const bitset& rr) noexcept {
            bitset res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), [](_Sty b1, _Sty b2) { return b1 & ~b2; });
            return res;
        }

        template <typename _CharT, typename _Traits>
        friend std::basic_ostream<_CharT, _Traits>& operator <<(std::basic_ostream<_CharT, _Traits>& os, const bitset& r) {
            using _OsT = std::basic_ostream<_CharT, _Traits>;
            const typename _OsT::sentry ok{os};
            if (ok) {
                const auto& facet = std::use_facet<std::ctype<_CharT>>(os.getloc());
                const std::array<_CharT, 2> zo = { facet.widen('0'), facet.widen('1') };
                r.rfor_every([&os, zo](_Ty, bool b) {
                    os << zo[b];
                    });
            }
            return os;
        }

        template <typename _CharT, typename _Traits>
        friend std::basic_istream<_CharT, _Traits>& operator >>(std::basic_istream<_CharT, _Traits>& is, bitset& r) {
            detail::_temp<bitset> tmp;
            using _IsT = std::basic_istream<_CharT, _Traits>;
            const typename _IsT::sentry ok{is};
            typename _IsT::iostate state = _IsT::goodbit;
            bitset::std::size_t i = r.max_size();
            if (ok) {
                const auto& facet = std::use_facet<std::ctype<_CharT>>(is.getloc());
                const _CharT zero = facet.widen('0'), one = facet.widen('1');
                try {
                    typename _Traits::int_type meta = is.rdbuf()->sgetc();
                    while (i) {
                        if (_Traits::eq_int_type(_Traits::eof(), meta)) {
                            state |= _IsT::eofbit;
                            break;
                        }
                        else if (const _CharT c = _Traits::to_char_type(meta); c == one) {
                            tmp->_ref_pos(--i).set();
                        }
                        else if (c == zero) {
                            --i;
                        }
                        else {
                            break;
                        }
                        meta = is.rdbuf()->snextc();
                    }
                }
                catch (...) {
                    ok = false;
                    state |= _IsT::badbit;
                }
            }
            if (i == r.max_size()) {
                state |= _IsT::failbit;
            }
            is.setstate(state);
            if (ok) {
                if (i == r.max_size()) {
                    r.clear();
                }
                else {
                    arr_it dst = tmp->_shr(i, r.a.begin());
                    std::fill(dst, r.a.end(), 0);
                }
            }
            return is;
        }

    public:
        using set_t = ::irc::set<_Ty, _Rfirst, _Rlast, _Sty>;
        using unordered_set_t = ::irc::unordered_set<_Ty, _Rfirst, _Rlast, _Sty>;
        
        set_t& as_set() noexcept {
            return *static_cast<set_t*>(static_cast<_Base*>(this));
        }

        const set_t& as_set() const noexcept {
            return *static_cast<const set_t*>(static_cast<const _Base*>(this));
        }

        unordered_set_t& as_unordered_set() noexcept {
            return *static_cast<unordered_set_t*>(static_cast<_Base*>(this));
        }

        const unordered_set_t& as_unordered_set() const noexcept {
            return *static_cast<const unordered_set_t*>(static_cast<const _Base*>(this));
        }
    };
}

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
inline constexpr bool std::ranges::disable_sized_range<irc::bitset<_Ty, _Rfirst, _Rlast, _Sty>> = !irc::detail::bitset<_Ty, _Rfirst, _Rlast, _Sty>::is_single_bucket;

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
struct std::hash<irc::bitset<_Ty, _Rfirst, _Rlast, _Sty>> {
    constexpr std::size_t operator ()(const irc::bitset<_Ty, _Rfirst, _Rlast, _Sty>& r) const noexcept {
        return std::hash<typename irc::bitset<_Ty, _Rfirst, _Rlast, _Sty>::internal_array_type>{}(r.as_array());
    }
};

// Aliases
namespace irc {

    template <
        auto _R1,
        auto _R2 = nullptr,
        typename _Sty = detail::_fast_sty_for_range_t<
            typename detail::_deduce_range_t<_R1, _R2>::item_type,
            detail::_deduce_range_t<_R1, _R2>::first,
            detail::_deduce_range_t<_R1, _R2>::last
        >
    >
    using auto_bitset = bitset<
        typename detail::_deduce_range_t<_R1, _R2>::item_type,
        detail::_deduce_range_t<_R1, _R2>::first,
        detail::_deduce_range_t<_R1, _R2>::last,
        _Sty
    >;

    template <
        typename _Enum,
        typename _Sty = detail::_fast_sty_for_range_t<
            _Enum,
            enum_range<_Enum>().first,
            enum_range<_Enum>().second
        >
    >
    using enum_bitset = bitset<
        _Enum,
        enum_range<_Enum>().first,
        enum_range<_Enum>().second,
        _Sty
    >;

    template <
        typename _IntT,
        typename _Sty = detail::_fast_sty_t<>
    >
    using max_bitset = bitset<
        _IntT,
        std::numeric_limits<_IntT>::min(),
        std::numeric_limits<_IntT>::max(),
        _Sty
    >;

    using bool_bitset = max_bitset<bool, std::uint8_t>;
}
