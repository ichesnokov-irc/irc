// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "basic_set_detail.hpp"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <functional>
#include <utility>
#include <initializer_list>
#include <optional>
#include <compare>
#include <iterator>
#include <array>
#include <string>
#include <string_view>
#include <iosfwd>
#include <locale>
#include <ranges>

namespace irc {

    template <typename _Ty>
    class set_nh {
    public:
        using value_type = _Ty;

        constexpr set_nh() noexcept : v{} {}
        constexpr set_nh(_Ty v) noexcept : v{v} {}
        constexpr set_nh& operator =(const set_nh&) = default;

        constexpr bool empty() const noexcept {
            return !v.has_value();
        }

        constexpr explicit operator bool() const noexcept {
            return v.has_value();
        }

        constexpr _Ty& value() const {
            assert(v.has_value());
            return v.value();
        }

        constexpr void swap(set_nh& nh) noexcept {
            std::swap(v, nh.v);
        }

    private:
        mutable std::optional<_Ty> v;
    };

    template <
        typename _Ty, _Ty _Rfirst, _Ty _Rlast,
        typename _Sty = detail::_fast_sty_for_range_t<_Ty, _Rfirst, _Rlast>
    >
    class basic_set {
        static_assert(std::is_same_v<std::decay_t<_Ty>, _Ty> && (std::is_integral_v<_Ty> || std::is_enum_v<_Ty>),
            "integral type (including bool) or enum is required");
        static_assert(std::is_same_v<_Sty, detail::_n_uint_t<sizeof(_Sty)>>,
            "storage item type (bucket) for irc::basic_set<> must be one of std::uintN_t");

        constexpr static std::uintmax_t _Max = static_cast<std::uintmax_t>(_Rlast) - static_cast<std::uintmax_t>(_Rfirst);
        constexpr static std::size_t _Bbs = CHAR_BIT;
        static_assert(_Bbs == 8); // apocalypse check

        using _Tu = detail::_underlying_uint_t<_Ty>;
        using _Ti = detail::_n_uint_t<std::max({sizeof(_Tu), sizeof(int), sizeof(detail::_up_uint_t<_Max>)})>;
        static_assert(sizeof(_Ty) == sizeof(_Tu) && sizeof(_Ti) >= sizeof(_Tu));

        constexpr static _Tu _Ufirst = static_cast<_Tu>(_Rfirst);
        constexpr static _Tu _Ulast = static_cast<_Tu>(_Rlast);
        static_assert(_Ufirst <= _Ulast, "Invalid range");

        constexpr static _Ti _BktBytes = sizeof(_Sty);
        constexpr static _Ti _BktBits = _BktBytes * _Bbs;

        static_assert(_Max < std::numeric_limits<_Ti>::max(), "Too big range");
        constexpr static _Ti _Range = static_cast<_Ti>(_Max) + 1;
        constexpr static _Ti _NBkt = (_Range / _BktBits) + !!(_Range % _BktBits);
        static_assert(_NBkt >= 1);

        constexpr static _Ti _Inext = _BktBits;
        constexpr static _Ti _Imask = _Inext - 1;
        constexpr static _Ti _INmask = ~_Imask;
        constexpr static _Ti _IEnd = _NBkt * _BktBits;
        constexpr static _Ti _ILast = _Range - 1;
        constexpr static unsigned int _Ibits = std::bit_width(_Imask);

        constexpr static _Sty _Every = ~_Sty{};
        constexpr static _Sty _One = 1u;
        constexpr static _Sty _None = _Every ^ _One;
        constexpr static _Sty _Bmask = _Range % _BktBits ? static_cast<_Sty>(~(_Every << (_Range % _BktBits))) : _Every;

        using arr_t = std::array<_Sty, _NBkt>;
        using arr_it = arr_t::iterator;
        using arr_cit = arr_t::const_iterator;

        constexpr static auto _range = std::views::iota(_Ti{}, _Range)
            | std::views::transform([](_Ti v) -> _Ty { return static_cast<_Ty>(v + _Ufirst); });

        class _iterator {
            constexpr static _iterator beg(arr_cit a) noexcept {
                if constexpr (_NBkt == 1) {
                    return { static_cast<_Ti>(std::countr_zero(*a)), a };
                } else {
                    _Ti v = 0;
                    arr_cit p = a;
                    _Sty b;
                    while (!(b = *p)) {
                        v += _Inext;
                        ++p;
                        if (v == _IEnd) {
                            return { _IEnd, p };
                        }
                    }
                    v |= static_cast<_Ti>(std::countr_zero(b));
                    assert(v < _IEnd);
                    return { v, p };
                }
            }

            constexpr static _iterator end(arr_cit ae) noexcept {
                return { _IEnd, ae };
            }

            constexpr void next() noexcept {
                assert(v < _IEnd);
                if constexpr (_NBkt == 1) {
                    const _Sty b = (*p) & (_None << v);
                    v = static_cast<_Ti>(std::countr_zero(b));
                } else {
                    const _Ti vl = v;
                    _Ti vn = vl & _INmask;
                    _Sty b = (*p) & (_None << (vl & _Imask));
                    while (!b) {
                        vn += _Inext, ++p;
                        if (vn == _IEnd) {
                            v = _IEnd;
                            return;
                        }
                        b = *p;
                    }
                    v = vn | static_cast<_Ti>(std::countr_zero(b));
                    assert(v <= _IEnd);
                }
            }

            constexpr void prev() noexcept {
                assert(v > 0 && v <= _IEnd);
                if constexpr (_NBkt == 1) {
                    const _Sty b = (*p) & (_Every >> (_IEnd - v));
                    assert(b); // Iterator underflow
                    v = static_cast<_Ti>(std::countl_zero(b)) ^ _Imask;
                } else {
                    _Ti vp = v;
                    _Sty b;
                    if (const _Ti vi = (vp & _Imask); !vi) {
                        b = 0;
                    } else {
                        b = (*p) & (_Every >> (_Inext - vi));
                        vp &= _INmask;
                    }
                    while (!b) {
                        assert(vp >= _Inext); // Iterator underflow
                        vp -= _Inext;
                        b = *(--p);
                    }
                    v = vp | (static_cast<_Ti>(std::countl_zero(b)) ^ _Imask);
                    assert(v <= _ILast);
                }
            }

            constexpr _Ty key(std::ptrdiff_t d = 0) const noexcept {
                return static_cast<_Ty>(v + _Ufirst + d);
            }

            constexpr _iterator(_Ti v, arr_cit p) noexcept : v{v}, p{p} {}

            _Ti v;
            arr_cit p;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = _Ty;
            using pointer = const _Ty*;
            using reference = const _Ty&;

            constexpr _iterator() noexcept : v{}, p{} {}

            [[nodiscard]] constexpr bool operator ==(const _iterator& other) const noexcept { return v == other.v; }
            [[nodiscard]] constexpr std::strong_ordering operator <=>(const _iterator& other) const noexcept { return v <=> other.v; }

            [[nodiscard]] constexpr _Ty operator *() const noexcept {
                assert(v <= _ILast); // Past-the-end or corrupted iterator
                return key();
            }

            constexpr _iterator& operator ++() noexcept {
                next();
                return *this;
            }

            constexpr _iterator operator ++(int) noexcept {
                const _iterator cpy{ *this };
                next();
                return cpy;
            }

            constexpr _iterator& operator --() noexcept {
                prev();
                return *this;
            }

            constexpr _iterator operator --(int) noexcept {
                const _iterator cpy{ *this };
                prev();
                return cpy;
            }

            friend basic_set;
        };

        class _local_iterator {
            constexpr _local_iterator(_Ti v, arr_cit p) noexcept : it{v, p} {}

            constexpr static _local_iterator beg(std::size_t i, arr_cit a) noexcept {
                _local_iterator lit{i * _Inext, std::next(a, i)};
                const _Sty b = *lit.it.p;
                if (!(b & _One)) [[likely]] {
                    lit.it.v += static_cast<_Ti>(std::countr_zero(b));
                    lit.it.p += !b;
                }
                return lit;
            }

            constexpr static _local_iterator end(_Ti i, arr_cit a) noexcept {
                return {(i + 1) * _Inext, std::next(a, i + 1)};
            }
            
            constexpr void next() noexcept {
                assert(it.v < _IEnd);
                const _Ti vl = it.v;
                const _Ti v = vl & _INmask;
                const _Sty b = (*it.p) & (_None << (vl & _Imask));
                if (!b) {
                    (it.v = v + _Inext), ++it.p;
                } else {
                    it.v = v | static_cast<_Ti>(std::countr_zero(b));
                }
                assert(it.v <= _IEnd);
            }

            _iterator it;

        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = _iterator::difference_type;
            using value_type = _iterator::value_type;
            using pointer = _iterator::pointer;
            using reference = _iterator::reference;

            constexpr _local_iterator() noexcept = default;

            [[nodiscard]] constexpr bool operator ==(const _local_iterator& other) const noexcept { return it == other.it; }
            [[nodiscard]] constexpr std::strong_ordering operator <=>(const _local_iterator& other) const noexcept { return it <=> other.it; }

            [[nodiscard]] constexpr _Ty operator *() const noexcept {
                return *it;
            }

            [[nodiscard]] constexpr operator _iterator() const noexcept {
                return it;
            }

            constexpr _local_iterator& operator ++() noexcept {
                next();
                return *this;
            }

            constexpr _local_iterator operator ++(int) noexcept {
                const _local_iterator cpy{*this};
                next();
                return cpy;
            }

            friend basic_set;
        };

        template <typename _ItA>
        class _ref_base {
            constexpr _ref_base(_Sty i, _ItA p) : i{i}, p{p} {}
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
                    return _ref_base{_One << v, a};
                } else if constexpr (std::bit_width(_IEnd) < sizeof(int) * _Bbs) {
                    return _ref_base{std::rotl(_One, static_cast<int>(v)), std::next(a, static_cast<difference_type>(v >> _Ibits))};
                } else {
                    return _ref_base{_One << (v & _Imask), std::next(a, v >> _Ibits)};
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

            friend basic_set;
        };

        class _reference : public _ref_base<arr_it> {
            using _base = _ref_base<arr_it>;
            constexpr _reference(_Sty i, arr_it p) noexcept : _base{i, p} {}
            constexpr explicit _reference(const _base& r) noexcept : _base{r} {}

            constexpr static _reference at(_Ty v, arr_it a) noexcept {
                return _reference{_base::at(v, a)};
            }

            constexpr static _reference at_pos(_Ti v, arr_it a) noexcept {
                return _reference{_base::at_pos(v, a)};
            }

        public:
            constexpr _reference& operator =(bool x) noexcept {
                if (x) {
                    set();
                } else {
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

            friend basic_set;
        };

        struct _insert_return_type {
            const _iterator position;
            bool inserted;
            set_nh<_Ty> node;
        };

        arr_t a;

    public:
        using internal_array_type = arr_t;
        using key_type = _Ty;
        using value_type = _Ty;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using key_compare = std::less<>;
        using value_compare = std::less<>;
        using hasher = std::hash<_Ty>;
        using key_equal = std::equal_to<>;
        using pointer = _Ty*;
        using const_pointer = const _Ty*;
        using iterator = _iterator;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using local_iterator = std::conditional_t<_NBkt == 1, _iterator, _local_iterator>;
        using const_local_iterator = local_iterator;
        using node_type = set_nh<_Ty>;
        using reference = _reference;
        using const_reference = _ref_base<arr_cit>;
        using insert_return_type = _insert_return_type;

        constexpr basic_set() noexcept : a{} {}
        constexpr basic_set(const basic_set&) noexcept = default;
        constexpr basic_set(basic_set&&) noexcept = default;

        template <typename _InIt>
        constexpr basic_set(_InIt first, _InIt last) : a{} {
            _set_from(first, last);
        }

        constexpr basic_set(std::initializer_list<value_type> init) noexcept : a{} {
            _set_from(init.begin(), init.end());
        }

        constexpr basic_set& operator =(const basic_set&) noexcept = default;
        constexpr basic_set& operator =(basic_set&&) noexcept = default;

        constexpr basic_set& operator =(std::initializer_list<value_type> init) noexcept {
            clear();
            _set_from(init.begin(), init.end());
            return *this;
        }

        [[nodiscard]] constexpr iterator begin() noexcept {
            return _beg();
        }

        [[nodiscard]] constexpr const_iterator begin() const noexcept {
            return _beg();
        }

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
            return _beg();
        }

        [[nodiscard]] constexpr iterator end() noexcept {
            return _end();
        }

        [[nodiscard]] constexpr const_iterator end() const noexcept {
            return _end();
        }

        [[nodiscard]] constexpr const_iterator cend() const noexcept {
            return _end();
        }

        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
            return std::make_reverse_iterator(_end());
        }

        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
            return std::make_reverse_iterator(_end());
        }

        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
            return std::make_reverse_iterator(_end());
        }

        [[nodiscard]] constexpr reverse_iterator rend() noexcept {
            return std::make_reverse_iterator(_beg());
        }

        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
            return std::make_reverse_iterator(_beg());
        }

        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
            return std::make_reverse_iterator(_beg());
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return std::none_of(a.cbegin(), a.cend(), [](_Sty b) { return b; });
        }

        [[nodiscard]] constexpr size_type size() const noexcept {
            return std::transform_reduce(a.cbegin(), a.cend(), size_type{}, std::plus<>{},
                [](_Sty b) { return std::popcount(b); });
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return _Range;
        }

        constexpr void clear() noexcept {
            a.fill(0);
        }

        constexpr std::pair<iterator, bool> insert(const value_type& value) noexcept {
            assert(_in_range(value));
            reference ref = _ref(value);
            const bool inserted = !ref.test_and_set();
            return {_to_it(ref, value), inserted};
        }

        constexpr std::pair<iterator, bool> insert(value_type&& value) noexcept {
            return insert(static_cast<value_type&>(value));
        }

        constexpr iterator insert(const_iterator pos, const value_type& value) noexcept {
            assert(_is_valid(pos));
            assert(_in_range(value));
            reference ref = _ref(value);
            ref.set();
            return _to_it(ref, value);
        }

        constexpr iterator insert(const_iterator pos, value_type&& value) noexcept {
            return insert(pos, static_cast<value_type&>(value));
        }

        template <typename _InIt>
        constexpr void insert(_InIt first, _InIt last) {
            _set_from(first, last);
        }

        constexpr void insert(std::initializer_list<value_type> init) noexcept {
            _set_from(init.begin(), init.end());
        }

        constexpr insert_return_type insert(node_type&& nh) noexcept {
            if (nh.empty()) [[unlikely]] {
                return {_end(), false, {}};
            }
            const auto [it, inserted] = insert(nh.value());
            return {it, inserted, nh};
        }

        constexpr iterator insert(const_iterator pos, node_type&& nh) noexcept {
            if (nh.empty()) [[unlikely]] {
                return _end();
            }
            return insert(pos, nh.value());
        }

        template <typename... Args>
        constexpr std::pair<iterator, bool> emplace(Args&&... args) noexcept(noexcept(value_type{std::forward<Args>(args)...})) {
            const value_type value{std::forward<Args>(args)...};
            return insert(value);
        }

        template <typename... Args>
        constexpr iterator emplace_hint(const_iterator pos, Args&&... args) noexcept(noexcept(value_type{std::forward<Args>(args)...})) {
            const value_type value{std::forward<Args>(args)...};
            return insert(pos, value);
        }

        constexpr iterator erase(const_iterator pos) noexcept {
            assert(_is_valid(pos));
            _to_ref(pos).reset();
            return std::next(pos);
        }

        constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
            assert(_is_valid(first) && _is_valid(last) && _is_ordered(first, last));
            if (first != last) [[likely]] {
                fill<0>(first.key(), last.key(-1));
            }
            return last;
        }

        constexpr size_type erase(const key_type& key) noexcept {
            if (!_in_range(key)) {
                return 0;
            }
            return _ref(key).test_and_reset();
        }

        constexpr void swap(basic_set& other) noexcept {
            a.swap(other.a);
        }

        constexpr node_type extract(const_iterator pos) noexcept {
            assert(_is_valid(pos));
            _ref(*pos).reset();
            return {*pos};
        }

        constexpr node_type extract(const key_type& key) noexcept {
            if (!_in_range(key) || !_ref(key).test_and_reset()) {
                return {};
            }
            return {key};
        }

        constexpr void merge(basic_set& src) noexcept {
            std::transform(a.begin(), a.end(), src.a.begin(), a.begin(), [](_Sty b1, _Sty& b2) {
                const _Sty m = b1 & b2;
                b1 |= b2;
                b2 &= ~m;
                return b1;
            });
        }

        constexpr void merge(basic_set&& src) noexcept {
            // we do not modify the src because its assumed to be moved in
            std::transform(a.begin(), a.end(), src.begin(), a.begin(), std::bit_or);
        }

        [[nodiscard]] constexpr const_iterator find(const key_type& key) const noexcept {
            if (!_in_range(key)) {
                return _end();
            }
            const_reference ref = _ref(key);
            return ref.test() ? _to_it(ref, key) : _end();
        }

        [[nodiscard]] constexpr iterator find(const key_type& key) noexcept {
            return _const().find(key);
        }

        template <typename K>
        [[nodiscard]] constexpr iterator find(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            const_iterator it = lower_bound<K>(x);
            key_compare less{};
            return it != _end() && !less(*it, x) && !less(x, *it) ? it : _end();
        }

        template <typename K>
        [[nodiscard]] constexpr iterator find(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().find<K>(x);
        }

        [[nodiscard]] constexpr bool contains(const key_type& key) const noexcept {
            return _in_range(key) && _ref(key).test();
        }

        template <typename K>
        [[nodiscard]] constexpr bool contains(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            return std::ranges::binary_search(_range, x, key_compare{});
        }

        [[nodiscard]] constexpr size_type count(const key_type& key) const noexcept {
            return contains(key);
        }

        template <typename K>
        [[nodiscard]] constexpr size_type count(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            return contains<K>(x);
        }

        [[nodiscard]] constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const noexcept {
            const _Tu u = static_cast<_Tu>(key);
            if (u < _Ufirst) [[unlikely]] {
                iterator bg = _beg();
                return {bg, bg};
            } else if (u > _Ulast) [[unlikely]] {
                return {_end(), _end()};
            }
            const_reference ref = _ref(key);
            const_iterator it = _to_it(ref, key);
            if (!ref.test()) {
                it.next();
                return {it, it};
            }
            return {it, std::next(it)};
        }

        [[nodiscard]] constexpr std::pair<iterator, iterator> equal_range(const key_type& key) noexcept {
            return _const().equal_range(key);
        }

        template <typename K>
        [[nodiscard]] constexpr std::pair<const_iterator, const_iterator> equal_range(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            const auto [first, last] = std::ranges::equal_range(_range, x, key_compare{});
            std::pair<const_iterator, const_iterator> r;
            if (first == _range.end()) {
                r.first = _end();
            } else if (const_reference rfirst = _ref(*first); rfirst.test()) {
                r.first = _to_it(rfirst, *first);
            } else {
                r.first = std::next(_to_it(rfirst, *first));
            }
            if (last == _range.end()) {
                r.second = _end();
            } else if (const_reference rlast = _ref(*last); rlast.test()) {
                r.second = _to_it(rlast, *last);
            } else {
                r.second = std::next(_to_it(rlast, *last));
            }
            return r;
        }

        template <typename K>
        [[nodiscard]] constexpr std::pair<iterator, iterator> equal_range(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().equal_range<K>(x);
        }

        [[nodiscard]] constexpr const_iterator lower_bound(const key_type& key) const noexcept {
            const _Tu u = static_cast<_Tu>(key);
            if (u < _Ufirst) [[unlikely]] {
                return _beg();
            } else if (u > _Ulast) [[unlikely]] {
                return _end();
            }
            const_reference ref = _ref(key);
            const_iterator it = _to_it(ref, key);
            return ref.test() ? it : std::next(it);
        }

        [[nodiscard]] constexpr iterator lower_bound(const key_type& key) noexcept {
            return _const().lower_bound(key);
        }
        
        template <typename K>
        [[nodiscard]] constexpr const_iterator lower_bound(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            const auto first = std::ranges::lower_bound(_range, x, key_compare{});
            if (first == _range.end()) {
                return _end();
            } else if (const_reference rfirst = _ref(*first); rfirst.test()) {
                return _to_it(rfirst, *first);
            } else {
                return std::next(_to_it(rfirst, *first));
            }
        }
        
        template <typename K>
        [[nodiscard]] constexpr iterator lower_bound(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().lower_bound<K>(x);
        }

        [[nodiscard]] constexpr const_iterator upper_bound(const key_type& key) const noexcept {
            const_iterator it = lower_bound(key);
            return it.key() == key ? std::next(it) : it;
        }

        [[nodiscard]] constexpr iterator upper_bound(const key_type& key) noexcept {
            return _const().upper_bound(key);
        }
        
        template <typename K>
        [[nodiscard]] constexpr const_iterator upper_bound(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            const auto last = std::ranges::upper_bound(_range, x, key_compare{});
            if (last == _range.end()) {
                return _end();
            } else if (const_reference rlast = _ref(*last); rlast.test()) {
                return _to_it(rlast, *last);
            } else {
                return std::next(_to_it(rlast, *last));
            }
        }

        template <typename K>
        [[nodiscard]] constexpr iterator upper_bound(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().upper_bound<K>(x);
        }

        constexpr key_compare key_comp() const noexcept {
            return key_compare{};
        }

        constexpr value_compare value_comp() const noexcept {
            return value_compare{};
        }

    // std::unordered_set<_Ty> replacement helpers
    public:
        [[nodiscard]] constexpr local_iterator begin(size_type i) noexcept {
            return _beg(i);
        }

        [[nodiscard]] constexpr const_local_iterator begin(size_type i) const noexcept {
            return _beg(i);
        }

        [[nodiscard]] constexpr const_local_iterator cbegin(size_type i) const noexcept {
            return _beg(i);
        }

        [[nodiscard]] constexpr local_iterator end(size_type i) noexcept {
            return _end(i);
        }

        [[nodiscard]] constexpr const_local_iterator end(size_type i) const noexcept {
            return _end(i);
        }

        [[nodiscard]] constexpr const_local_iterator cend(size_type i) const noexcept {
            return _end(i);
        }

        constexpr hasher hash_function() const noexcept {
            return hasher{};
        }

        constexpr key_equal key_eq() const noexcept {
            return key_equal{};
        }

        constexpr void reshash() noexcept {
        }

        constexpr void reserve(size_type) noexcept {
        }

        [[nodiscard]] constexpr size_type bucket_count() const noexcept {
            return _NBkt;
        }

        [[nodiscard]] constexpr size_type max_bucket_count() const noexcept {
            return _NBkt;
        }

        [[nodiscard]] constexpr size_type bucket_size(size_type i) const noexcept {
            assert(i < bucket_count());
            return std::popcount(a[i]);
        }

        [[nodiscard]] constexpr size_type bucket(const key_type& key) const noexcept {
            const _Tu v = static_cast<_Tu>(key);
            assert(v >= _Ufirst && v <= _Ulast);
            return static_cast<size_type>(v - _Ufirst) / _BktBits;
        }

        [[nodiscard]] constexpr float load_factor() const noexcept {
            return static_cast<float>(size()) / (_NBkt * _BktBits);
        }

        [[nodiscard]] constexpr float max_load_factor() const noexcept {
            return static_cast<float>(max_size()) / (_NBkt * _BktBits);
        }

    // std::bitset<>-like methods
    public:
        constexpr explicit basic_set(unsigned long long val) noexcept : a{} {
            if (sizeof(_Sty) >= sizeof(unsigned long long) || std::bit_width(val) <= _BktBits) {
                _Sty v = static_cast<_Sty>(val);
                if constexpr (_Bmask != _Every && _NBkt == 1) {
                    v &= _Bmask;
                }
                a[0] = v;
            } else {
                arr_it it = a.begin();
                while (val != 0 && it != a.end()) {
                    *it++ = static_cast<_Sty>(val);
                    val >>= _BktBits;
                }
                a.back() &= _Bmask;
            }
        }

        template <typename _CharT>
        constexpr explicit basic_set(const _CharT* s, typename std::basic_string<_CharT>::size_type n = std::size_t(-1),
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : basic_set(
                n == std::basic_string<_CharT>::npos
                ? std::basic_string_view<_CharT>(s)
                : std::basic_string_view<_CharT>(s, n), 0, n, zero, one
            )
        {}

        template <typename _CharT, typename _Traits, typename _Alloc>
        constexpr explicit basic_set(const std::basic_string<_CharT, _Traits, _Alloc>& s,
            typename std::basic_string<_CharT, _Traits, _Alloc>::size_type pos = 0,
            typename std::basic_string<_CharT, _Traits, _Alloc>::size_type n = std::basic_string<_CharT, _Traits, _Alloc>::npos,
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : basic_set(
                static_cast<std::basic_string_view<_CharT, _Traits>>(s), pos, n, zero, one
            )
        {}

        template <typename _CharT, typename _Traits>
        constexpr explicit basic_set(const std::basic_string_view<_CharT, _Traits>& s,
            typename std::basic_string_view<_CharT, _Traits>::size_type pos = 0,
            typename std::basic_string_view<_CharT, _Traits>::size_type n = std::size_t(-1),
            _CharT zero = _CharT('0'), _CharT one = _CharT('1')) : a{} {
            const size_type end = std::min({n, _Range, s.length() - pos});
            const auto ss = s.substr(pos, end);
            auto&& it = ss.crbegin();
            for (size_type i = 0; i != end; ++i) {
                const _CharT c = *it++;
                if (!(_Traits::eq(c, zero) || _Traits::eq(c, one))) {
                    throw std::invalid_argument{};
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
            if (width() > sizeof(ulong) * _Bbs) {
                throw std::overflow_error{};
            }
            if constexpr (sizeof(_Sty) >= sizeof(ulong)) {
                return static_cast<ulong>(a[0]);
            } else {
                ulong r = 0;
                size_type i = std::min(a.size(), sizeof(ulong) / sizeof(_Sty));
                while (i-- != 0) {
                    r <<= _BktBits;
                    r |= a[i];
                }
                return r;
            }
        }

        [[nodiscard]] constexpr unsigned long long to_ullong() const {
            using ullong = unsigned long long;
            if (width() > sizeof(ullong) * _Bbs) {
                throw std::overflow_error{};
            }
            if constexpr (sizeof(_Sty) >= sizeof(ullong)) {
                return static_cast<ullong>(a[0]);
            } else {
                ullong r = 0;
                size_type i = std::min(a.size(), sizeof(ullong) / sizeof(_Sty));
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
                throw std::out_of_range{};
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

        [[nodiscard]] constexpr size_type count() const noexcept {
            return size();
        }

        constexpr basic_set& operator &=(const basic_set& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_and{});
            return *this;
        }

        constexpr basic_set& operator |=(const basic_set& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_or{});
            return *this;
        }

        constexpr basic_set& operator ^=(const basic_set& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), std::bit_xor{});
            return *this;
        }

        constexpr basic_set& operator -=(const basic_set& other) noexcept {
            std::transform(a.cbegin(), a.cend(), other.a.cbegin(), a.begin(), [](_Sty b1, _Sty b2) { return b1 & ~b2; });
            return *this;
        }

        constexpr basic_set operator ~() const noexcept {
            basic_set result;
            std::transform(a.cbegin(), std::prev(a.cend()), result.a.begin(), [](_Sty b) { return ~b; });
            result.a.back() = ~a.back() & _Bmask;
            return result;
        }

        constexpr basic_set& operator <<=(size_type pos) noexcept {
            if (pos >= _Range) {
                clear();
            } else if (pos != 0) [[likely]] {
                arr_it dst = _shl(pos, a.end());
                std::fill(a.begin(), dst, 0);
                a.back() &= _Bmask;
            }
            return *this;
        }

        constexpr basic_set operator <<(size_type pos) const noexcept {
            basic_set res;
            if (pos < _Range) {
                _shl(pos, res.a.end());
                res.a.back() &= _Bmask;
            }
            return res;
        }

        constexpr basic_set& operator >>=(size_type pos) noexcept {
            if (pos >= _Range) {
                clear();
            } else if (pos != 0) [[likely]] {
                arr_it dst = _shr(pos, a.begin());
                std::fill(dst, a.end(), 0);
            }
            return *this;
        }

        constexpr basic_set operator >>(size_type pos) const noexcept {
            basic_set res;
            if (pos < _Range) {
                _shr(pos, res.a.begin());
            }
            return res;
        }

        constexpr basic_set& set() noexcept {
            std::fill(a.begin(), std::prev(a.end()), _Every);
            a.back() = _Bmask;
            return *this;
        }

        constexpr basic_set& set(_Ty pos, bool value = true) {
            if (!_in_range(pos)) {
                throw std::out_of_range{};
            }
            _ref(pos) = value;
            return *this;
        }

        constexpr basic_set& reset() noexcept {
            clear();
            return *this;
        }

        constexpr basic_set& reset(_Ty pos) {
            if (!_in_range(pos)) {
                throw std::out_of_range{};
            }
            _ref(pos).reset();
            return *this;
        }

        constexpr basic_set& flip() noexcept {
            std::transform(a.cbegin(), std::prev(a.cend()), a.begin(), [](_Sty b) { return ~b; });
            a.back() ^= _Bmask;
            return *this;
        }

        constexpr basic_set& flip(_Ty pos) {
            if (!_in_range(pos)) {
                throw std::out_of_range{};
            }
            _ref(pos).flip();
            return *this;
        }

    // Extra methods
    public:
        constexpr explicit basic_set(const internal_array_type& a) noexcept : a{a} {
            a.back() &= _Bmask;
        }

        constexpr explicit basic_set(const _Sty* a, size_type n = array_size()) noexcept : a{} {
            std::copy_n(a, std::min(_NBkt, n), this->a.begin());
            a.back() &= _Bmask;
        }

        [[nodiscard]] constexpr static size_type array_size() noexcept {
            return _NBkt;
        }

        [[nodiscard]] constexpr static _Sty back_mask() noexcept {
            return _Bmask;
        }

        [[nodiscard]] constexpr const internal_array_type& as_array() const noexcept {
            return a;
        }

        [[nodiscard]] constexpr internal_array_type& as_array() noexcept {
            // Important notice: do not modify bits in the last() item of internal array outside the back_mask().
            // Setting any bit to 1 outside the back_mask() will lead to UB in most of owning container's methods.
            return a;
        }

        [[nodiscard]] constexpr const _Sty* data() const noexcept {
            return a.data();
        }

        [[nodiscard]] constexpr _Sty* data() noexcept {
            // Important notice: do not modify bits in the last() item of internal array outside the back_mask().
            // Setting any bit to 1 outside the back_mask() will lead to UB in most of owning container's methods.
            return a.data();
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

        template <bool _01, typename _UnaryPred>
        constexpr size_type flip_if(_UnaryPred&& pred) noexcept(std::is_nothrow_invocable_v<_UnaryPred, _Ty>) {
            size_type n = 0;
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
            } else {
                std::transform(a.cbegin(), std::prev(a.cend()), a.begin(), [&_Iter](_Sty b) { return ~_Iter(~b); });
                a.back() = ~_Iter(~a.back() & _Bmask) & _Bmask;
            }
            return n;
        }

        template <typename _BinaryPred>
        constexpr size_type flip_if(_BinaryPred&& pred) noexcept(std::is_nothrow_invocable_v<_BinaryPred, _Ty, const_reference>) {
            size_type n = 0;
            arr_it p = a.begin();
            _Sty i = _One;
            _Tu u = _Ufirst;
            do {
                if (pred(static_cast<_Ty>(u), const_reference{i, p})) {
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
            } else {
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
            } else {
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
                fn(static_cast<_Ty>(u), const_reference{i, p});
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
            } else {
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
                fn(static_cast<_Ty>(u), const_reference{i, p});
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
            } else {
                u1 = std::max(u1, _Ufirst);
            }
            assert(u1 <= u2);
            const reference r1 = _ref(static_cast<_Ty>(u1));
            if (u2 >= _Ulast) {
                const _Sty m = r1.i - 1u;
                if constexpr (_01) {
                    *r1.p |= ~m;
                    std::fill(std::next(r1.p), a.end(), _Every);
                } else {
                    *r1.p &= m;
                    std::fill(std::next(r1.p), a.end(), 0);
                }
            } else {
                const reference r2 = _ref(static_cast<_Ty>(u2));
                const _Sty m1 = r1.i - 1u;
                const _Sty m2 = (r2.i - 1u) | r2.i;
                if constexpr (_01) {
                    if (r1.p == r2.p) {
                        *r1.p |= m2 ^ m1;
                    } else {
                        *r1.p |= ~m1;
                        *r2.p |= m2;
                        std::fill(std::next(r1.p), r2.p, _Every);
                    }
                } else {
                    if (r1.p == r2.p) {
                        *r1.p &= ~(m2 ^ m1);
                    } else {
                        *r1.p &= m1;
                        *r2.p &= ~m2;
                        std::fill(std::next(r1.p), r2.p, 0);
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
            size_type i = 0;
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

        [[nodiscard]] constexpr size_type width() const noexcept {
            _Ti w = _IEnd;
            for (auto it = a.crbegin(); it != a.crend(); ++it) {
                if (*it) {
                    w -= std::countl_zero(*it);
                    break;
                } else {
                    w -= _BktBits;
                }
            }
            return w;
        }

    // Inspired by boost::dynamic_bitset
    public:
        [[nodiscard]] constexpr bool intersects(const basic_set& other) const noexcept {
            for (arr_cit it1 = a.cbegin(), it2 = other.a.cbegin(); it1 != a.cend(); ++it1, ++it2) {
                if (*it1 & *it2) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] constexpr bool is_subset_of(const basic_set& superset) const noexcept {
            for (arr_cit it1 = a.cbegin(), it2 = superset.a.cbegin(); it1 != a.cend(); ++it1, ++it2) {
                if ((*it1 & *it2) != *it1) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] constexpr bool is_proper_subset_of(const basic_set& superset) const noexcept {
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
        constexpr iterator _beg() const noexcept {
            return iterator::beg(a.cbegin());
        }

        constexpr iterator _end() const noexcept {
            if constexpr (_NBkt == 1) {
                return iterator::end(a.cbegin());
            } else {
                return iterator::end(a.cend());
            }
        }

        constexpr local_iterator _beg(size_type i) const noexcept {
            assert(i < bucket_count());
            if constexpr (_NBkt == 1) {
                return _beg();
            } else {
                return local_iterator::beg(i, a.cbegin());
            }
        }

        constexpr local_iterator _end(size_type i) const noexcept {
            assert(i < bucket_count());
            if constexpr (_NBkt == 1) {
                return _end();
            } else {
                return local_iterator::end(i, a.cbegin());
            }
        }

        constexpr reference _ref(_Ty v) noexcept {
            return reference::at(v, a.begin());
        }

        constexpr const_reference _ref(_Ty v) const noexcept {
            return const_reference::at(v, a.cbegin());
        }

        constexpr reference _ref_pos(size_type pos) noexcept {
            return reference::at_pos(pos, a.begin());
        }

        constexpr const_reference _to_ref(const iterator& it) const noexcept {
            return const_reference::at_pos(it.v, a.begin());
        }

        constexpr reference _to_ref(const iterator& it) noexcept {
            return reference::at_pos(it.v, a.begin());
        }

        template <typename _ItA>
        constexpr iterator _to_it(const _ref_base<_ItA>& ref, _Ty v) const noexcept {
            iterator it{static_cast<_Ti>(static_cast<_Tu>(v) - _Ufirst), ref.p};
            assert(_is_valid(it));
            return it;
        }

        constexpr bool _is_valid(const iterator& it) const noexcept {
            if (!(it.p >= a.cbegin() && it.p <= a.cend())) {
                return false;
            }
            if (!(it.v <= _ILast || it.v == _IEnd)) {
                return false;
            }
            return true;
        }

        constexpr bool _is_ordered(const iterator& first, const iterator& last) const noexcept {
            return first.v <= last.v;
        }

        template <typename _InIt>
        constexpr void _set_from(_InIt first, const _InIt last) noexcept {
            std::for_each<_InIt>(first, last, [this](const auto& v) {
                assert(_in_range(v));
                _ref(v).set();
            });
        }

        constexpr static bool _in_range(const _Ty& v) noexcept {
            const _Tu u = static_cast<_Tu>(v);
            return u >= _Ufirst && u <= _Ulast;
        }

        constexpr const basic_set& _const() noexcept {
            return *this;
        }

        template <typename K>
        consteval static bool _is_nothrow_cmp() noexcept {
            return noexcept(key_compare{}(std::declval<_Ty>(), std::declval<K>()))
                && noexcept(key_compare{}(std::declval<K>(), std::declval<_Ty>()))
                && noexcept(key_compare{}(std::declval<K>(), std::declval<K>()));
        }

        constexpr arr_it _shl(size_type pos, arr_it dst) const noexcept {
            assert(pos < _Range);
            if constexpr (_NBkt == 1) {
                *(--dst) = (pos >= _BktBits ? 0 : a[0] << pos);
            } else {
                const size_type shl_by = pos >> _Ibits;
                const size_type shl_bi = pos & _Imask;
                arr_cit src = std::prev(a.cend(), shl_by);
                if (shl_bi) {
                    const size_type br = _BktBits - shl_bi;
                    size_type n = std::distance(a.cbegin(), src);
                    while (--n) {
                        --dst, --src;
                        *dst = (*src << shl_bi) | (*std::prev(src) >> br);
                    }
                    *(--dst) = *std::prev(src) << shl_bi;
                } else {
                    dst = std::copy_backward(a.cbegin(), src, dst);
                }
            }
            return dst;
        }

        constexpr arr_it _shr(size_type pos, arr_it dst) const noexcept {
            assert(pos < _Range);
            if constexpr (_NBkt == 1) {
                *dst++ = (pos >= _BktBits ? 0 : a[0] >> pos);
            } else {
                const size_type shr_by = pos >> _Ibits;
                const size_type shr_bi = pos & _Imask;
                arr_cit src = std::next(a.cbegin(), shr_by);
                if (shr_bi) {
                    const size_type bl = _BktBits - shr_bi;
                    size_type n = std::distance(src, a.cend());
                    while (--n) {
                        *dst = (*src >> shr_bi) | (*std::next(src) << bl);
                        ++dst, ++src;
                    }
                    *dst++ = *src >> shr_bi;
                } else {
                    dst = std::copy(src, a.cend(), dst);
                }
            }
            return dst;
        }

    public:
        friend constexpr static bool operator ==(const basic_set& lr, const basic_set& rr) noexcept {
            return lr.a == rr.a;
        }

        friend constexpr static std::strong_ordering operator <=>(const basic_set& lr, const basic_set& rr) noexcept {
            return lr.a <=> rr.a;
        }
    
        friend constexpr static basic_set operator &(const basic_set& lr, const basic_set& rr) noexcept {
            basic_set res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_and{});
            return res;
        }

        friend constexpr static basic_set operator |(const basic_set& lr, const basic_set& rr) noexcept {
            basic_set res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_or{});
            return res;
        }

        friend constexpr static basic_set operator ^(const basic_set& lr, const basic_set& rr) noexcept {
            basic_set res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), std::bit_xor{});
            return res;
        }

        friend constexpr static basic_set operator -(const basic_set& lr, const basic_set& rr) noexcept {
            basic_set res;
            std::transform(lr.a.cbegin(), lr.a.cend(), rr.a.cbegin(), res.a.begin(), [](_Sty b1, _Sty b2) { return b1 & ~b2; });
            return res;
        }

        template <typename _CharT, typename _Traits>
        friend static std::basic_ostream<_CharT, _Traits>& operator <<(std::basic_ostream<_CharT, _Traits>& os, const basic_set& r) {
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
        friend static std::basic_istream<_CharT, _Traits>& operator >>(std::basic_istream<_CharT, _Traits>& is, basic_set& r) {
            detail::_temp<basic_set> tmp;
            using _IsT = std::basic_istream<_CharT, _Traits>;
            const typename _IsT::sentry ok{is};
            typename _IsT::iostate state = _IsT::goodbit;
            basic_set::size_type i = r.max_size();
            if (ok) {
                const auto& facet = std::use_facet<std::ctype<_CharT>>(is.getloc());
                const _CharT zero = facet.widen('0'), one = facet.widen('1');
                try {
                    typename _Traits::int_type meta = is.rdbuf()->sgetc();
                    while (i) {
                        if (_Traits::eq_int_type(_Traits::eof(), meta)) {
                            state |= _IsT::eofbit;
                            break;
                        } else if (const _CharT c = _Traits::to_char_type(meta); c == one) {
                            tmp->_ref_pos(--i).set();
                        } else if (c == zero) {
                            --i;
                        } else {
                            break;
                        }
                        meta = is.rdbuf()->snextc();
                    }
                } catch (...) {
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
                } else {
                    arr_it dst = tmp->_shr(i, r.a.begin());
                    std::fill(dst, r.a.end(), 0);
                }
            }
            return is;
        }
    };

    template <typename _Ty>
    constexpr void swap(set_nh<_Ty>& lnh, set_nh<_Ty>& rnh) noexcept {
        lnh.swap(rnh);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
    constexpr void swap(basic_set<_Ty, _Rfirst, _Rlast, _Sty>& lr, basic_set<_Ty, _Rfirst, _Rlast, _Sty>& rr) noexcept {
        lr.swap(rr);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty, typename _V>
    constexpr std::size_t erase(basic_set<_Ty, _Rfirst, _Rlast, _Sty>& r, const _V& v) noexcept {
        return r.erase(v);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty, typename _UnaryPred>
    constexpr std::size_t erase_if(basic_set<_Ty, _Rfirst, _Rlast, _Sty>& r, _UnaryPred&& pred) noexcept {
        return r.flip_if<1>(std::forward<_UnaryPred>(pred));
    }
}

namespace std {
    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
    struct hash<irc::basic_set<_Ty, _Rfirst, _Rlast, _Sty>> {
        inline constexpr std::size_t operator()(const irc::basic_set<_Ty, _Rfirst, _Rlast, _Sty>& r) const noexcept {
            return std::hash<typename irc::basic_set<_Ty, _Rfirst, _Rlast, _Sty>::internal_array_type>{}(r.as_array());
        }
    };
}
