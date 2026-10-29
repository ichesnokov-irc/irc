// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail/bitset_base.hpp"
#include "auto_range.hpp"
#include "enum_range.hpp"
#include "forward.hpp"

#include <cassert>
#include <algorithm>
#include <numeric>
#include <functional>
#include <optional>
#include <initializer_list>

namespace irc {

    template <typename _Ty>
    class set_node_handle {
    public:
        using value_type = _Ty;

        constexpr set_node_handle() noexcept : v{} {}
        constexpr set_node_handle(_Ty v) noexcept : v{v} {}
        
        constexpr set_node_handle(const set_node_handle&) = delete;
        constexpr set_node_handle& operator =(const set_node_handle&) = delete;

        constexpr set_node_handle(set_node_handle&& src) noexcept : v{std::move(src.v)} {
            src.v.reset();
        }

        constexpr set_node_handle& operator =(set_node_handle&& src) noexcept {
            v = std::move(src.v);
            src.v.reset();
            return *this;
        }

        constexpr bool empty() const noexcept {
            return !v.has_value();
        }

        constexpr explicit operator bool() const noexcept {
            return v.has_value();
        }

        constexpr const value_type& value() const noexcept {
            assert(v.has_value());
            return v.value();
        }

        constexpr value_type& value() noexcept {
            assert(v.has_value());
            return v.value();
        }

        constexpr void swap(set_node_handle& nh) noexcept {
            std::swap(v, nh.v);
        }

    private:
        std::optional<_Ty> v;
    };

    template <
        typename _Ty, _Ty _Rfirst, _Ty _Rlast,
        typename _Sty = detail::_fast_sty_for_range_t<_Ty, _Rfirst, _Rlast>
    >
    class set : protected detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty> {
        using _Base = detail::bitset_base<_Ty, _Rfirst, _Rlast, _Sty>;

    public:
        using
            _Base::_Base,
            _Base::operator =,
            _Base::is_single_bucket,
            _Base::internal_array_type,
            _Base::array_size,
            _Base::back_mask,
            _Base::as_array,
            _Base::first,
            _Base::last,
            _Base::range;
    
    private:
        class _iterator {
            constexpr static _iterator beg(arr_cit a) noexcept {
                if constexpr (is_single_bucket) {
                    return { static_cast<_Ti>(std::countr_zero(*a)), a };
                } else {
                    _Ti v = 0;
                    arr_cit p = a;
                    while (!*p) {
                        v += _Inext;
                        ++p;
                        if (v == _IEnd) {
                            return { _IEnd, p };
                        }
                    }
                    v |= static_cast<_Ti>(std::countr_zero(*p));
                    assert(v < _IEnd);
                    return { v, p };
                }
            }

            constexpr static _iterator end(arr_cit ae) noexcept {
                return { _IEnd, ae };
            }

            constexpr void next() noexcept {
                assert(v < _IEnd);
                if constexpr (is_single_bucket) {
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
                if constexpr (is_single_bucket) {
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

            constexpr _iterator(_Ti v, arr_cit p) noexcept : v{v}, p{p} {}

            _Ti v;
            arr_cit p;

        public:
            using iterator_concept = std::bidirectional_iterator_tag;
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const _Ty;
            using pointer = const _Ty*;
            using reference = const _Ty&;

            constexpr _iterator() noexcept : v{}, p{} {}

            [[nodiscard]] constexpr bool operator ==(const _iterator& other) const noexcept { return v == other.v; }
            [[nodiscard]] constexpr std::strong_ordering operator <=>(const _iterator& other) const noexcept { return v <=> other.v; }

            [[nodiscard]] constexpr value_type operator *() const noexcept {
                assert(v <= _ILast); // Past-the-end or corrupted iterator
                return static_cast<_Ty>(v + _Ufirst);
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

            friend class set;
        };

        struct _insert_return_type {
            const _iterator position;
            bool inserted;
            set_node_handle<_Ty> node;
        };

    public:
        using key_type = _Ty;
        using value_type = _Ty;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using key_compare = detail::transparency::_comparator<_Ty>;
        using value_compare = key_compare;
        using pointer = _Ty*;
        using const_pointer = const _Ty*;
        using iterator = _iterator;
        using const_iterator = _iterator;
        using reverse_iterator = std::reverse_iterator<_iterator>;
        using const_reverse_iterator = std::reverse_iterator<_iterator>;
        using node_type = set_node_handle<_Ty>;
        using reference = _Ty&;
        using const_reference = const _Ty&;
        using insert_return_type = _insert_return_type;

        constexpr set(set&& src) noexcept : _Base{src.a} {
            src.clear();
        }

        template <typename _InIt>
        constexpr set(_InIt first, _InIt last) : _Base{} {
            _set_from(first, last);
        }

        constexpr set(std::initializer_list<value_type> init) noexcept : _Base{} {
            _set_from(init.begin(), init.end());
        }

        constexpr set& operator =(set&& src) noexcept {
            a = std::move(src.a);
            src.clear();
            return *this;
        }

        constexpr set& operator =(std::initializer_list<value_type> init) noexcept {
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
            if constexpr (is_single_bucket) {
                return !a[0];
            } else {
                return std::none_of(a.cbegin(), a.cend(), [](_Sty b) { return b; });
            }
        }

        [[nodiscard]] constexpr size_type size() const noexcept {
            return _Base::count();
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return _Base::size();
        }

        constexpr void clear() noexcept {
            if constexpr (is_single_bucket) {
                a[0] = 0;
            } else {
                a.fill(0);
            }
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
            ((void)pos);
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
            if (inserted) {
                nh = {};
            }
            return {it, inserted, std::move(nh)};
        }

        constexpr iterator insert(const_iterator pos, node_type&& nh) noexcept {
            ((void)pos);
            assert(_is_valid(pos));
            if (nh.empty()) [[unlikely]] {
                return _end();
            }
            const auto [it, inserted] = insert(nh.value());
            if (inserted) {
                nh = {};
            }
            return it;
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
            assert(_is_valid_ne(pos));
            _to_ref(pos).reset();
            return std::next(pos);
        }

        constexpr iterator erase(const_iterator first, const_iterator last) noexcept {
            assert(_is_valid(first) && _is_valid(last) && _is_ordered(first, last));
            if (first != last && first != _end()) {
                fill<0>(*first, last == _end() ? _Rlast : static_cast<_Ty>(static_cast<_Tu>(*last) - 1));
            }
            return last;
        }

        constexpr size_type erase(const key_type& key) noexcept {
            if (!_in_range(key)) {
                return 0;
            }
            return _ref(key).test_and_reset();
        }

        template <typename K>
        constexpr size_type erase(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                return ord == 0 && erase(key);
            } else {
                iterator it = find<K>(x);
                return it != _end() && erase(*it);
            }
        }

        constexpr void swap(set& other) noexcept {
            a.swap(other.a);
        }

        constexpr node_type extract(const_iterator pos) noexcept {
            assert(_is_valid_ne(pos));
            _ref(*pos).reset();
            return {*pos};
        }

        constexpr node_type extract(const key_type& key) noexcept {
            if (!_in_range(key) || !_ref(key).test_and_reset()) {
                return {};
            }
            return {key};
        }

        constexpr void merge(set& src) noexcept {
            if (&src == this) {
                return;
            }
            std::transform(a.begin(), a.end(), src.a.begin(), a.begin(), [](_Sty b1, _Sty& b2) {
                const _Sty m = b1;
                b1 |= b2;
                b2 &= m;
                return b1;
            });
        }

        constexpr void merge(set&& src) noexcept {
            merge(static_cast<set&>(src));
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
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                return ord == 0 ? find(key) : _end();
            } else {
                const auto lb = std::ranges::lower_bound(_range, x, key_compare{});
                if (lb == _range.end()) {
                    return _end();
                } else if (const_reference r = _ref(*lb); r.test()) {
                    return _to_it(r, *lb);
                } else {
                    return _end();
                }
            }
        }

        template <typename K>
        [[nodiscard]] constexpr iterator find(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().template find<K>(x);
        }

        [[nodiscard]] constexpr bool contains(const key_type& key) const noexcept {
            return _in_range(key) && _ref(key).test();
        }

        template <typename K>
        [[nodiscard]] constexpr bool contains(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                return ord == 0 && _ref(key).test();
            } else {
                const auto lb = std::ranges::lower_bound(_range, x, key_compare{});
                return lb != _range.end() && _ref(*lb).test();
            }
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
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                if (ord < 0) {
                    iterator bg = _beg();
                    return {bg, bg};
                } else if (ord > 0) {
                    return {_end(), _end()};
                } else {
                    const_reference ref = _ref(key);
                    const_iterator it = _to_it(ref, key);
                    if (!ref.test()) {
                        it.next();
                        return {it, it};
                    }
                    return {it, std::next(it)};
                }
            } else {
                const auto [first, last] = std::ranges::equal_range(_range, x, key_compare{});
                std::pair<const_iterator, const_iterator> r;
                if (first == _range.end()) {
                    r.first = _end();
                } else {
                    const_reference rfirst = _ref(*first);
                    r.first = _to_it(rfirst, *first);
                    if (!rfirst.test()) {
                        r.first = std::next(r.first);
                    }
                }
                if (last == _range.end()) {
                    r.second = _end();
                } else {
                    const_reference rlast = _ref(*last);
                    r.second = _to_it(rlast, *last);
                    if (!rlast.test()) {
                        r.second = std::next(r.second);
                    }
                }
                return r;
            }
        }

        template <typename K>
        [[nodiscard]] constexpr std::pair<iterator, iterator> equal_range(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().template equal_range<K>(x);
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
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                if (ord < 0) {
                    return _beg();
                } else if (ord > 0) {
                    return _end();
                } else {
                    const_reference ref = _ref(key);
                    const_iterator it = _to_it(ref, key);
                    return ref.test() ? it : std::next(it);
                }
            } else {
                const auto lb = std::ranges::lower_bound(_range, x, key_compare{});
                const_iterator first;
                if (lb == _range.end()) {
                    first = _end();
                } else {
                    const_reference rfirst = _ref(*lb);
                    first = _to_it(rfirst, *lb);
                    if (!rfirst.test()) {
                        first = std::next(first);
                    }
                }
                return first;
            }
        }
        
        template <typename K>
        [[nodiscard]] constexpr iterator lower_bound(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().template lower_bound<K>(x);
        }

        [[nodiscard]] constexpr const_iterator upper_bound(const key_type& key) const noexcept {
            const_iterator it = lower_bound(key);
            return it != _end() && *it == key ? std::next(it) : it;
        }

        [[nodiscard]] constexpr iterator upper_bound(const key_type& key) noexcept {
            return _const().upper_bound(key);
        }
        
        template <typename K>
        [[nodiscard]] constexpr const_iterator upper_bound(const K& x) const noexcept(_is_nothrow_cmp<K>()) {
            if constexpr (_can_pass_through<K>()) {
                const auto [key, ord] = _try_pass_through(x);
                if (ord < 0) {
                    return _beg();
                } else if (ord > 0) {
                    return _end();
                } else {
                    return std::next(_to_it(_ref(key), key));
                }
            } else {
                const auto ub = std::ranges::upper_bound(_range, x, key_compare{});
                iterator last;
                if (ub == _range.end()) {
                    last = _end();
                } else {
                    const_reference rlast = _ref(*ub);
                    last = _to_it(rlast, *ub);
                    if (!rlast.test()) {
                        last = std::next(last);
                    }
                }
                return last;
            }
        }

        template <typename K>
        [[nodiscard]] constexpr iterator upper_bound(const K& x) noexcept(_is_nothrow_cmp<K>()) {
            return _const().template upper_bound<K>(x);
        }

        constexpr key_compare key_comp() const noexcept {
            return key_compare{};
        }

        constexpr value_compare value_comp() const noexcept {
            return value_compare{};
        }

    protected:
        constexpr iterator _beg() const noexcept {
            return iterator::beg(a.cbegin());
        }

        constexpr iterator _end() const noexcept {
            if constexpr (is_single_bucket) {
                return iterator::end(a.cbegin());
            } else {
                return iterator::end(a.cend());
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

        constexpr bool _is_valid_ne(const iterator& it) const noexcept {
            return _is_valid(it) && it.v != _IEnd;
        }

        constexpr bool _is_ordered(const iterator& first, const iterator& last) const noexcept {
            return first.v <= last.v;
        }

        template <typename _InIt>
        constexpr void _set_from(_InIt first, const _InIt last) noexcept {
            std::for_each<_InIt>(first, last, [this](const auto& v) noexcept {
                assert(_in_range(v));
                this->_ref(v).set();
            });
        }

        constexpr static bool _in_range(const _Ty& v) noexcept {
            const _Tu u = static_cast<_Tu>(v);
            return u >= _Ufirst && u <= _Ulast;
        }

        constexpr const set& _const() noexcept {
            return *this;
        }

        constexpr arr_it _shl(size_type pos, arr_it dst) const noexcept {
            assert(pos < _Range);
            if constexpr (is_single_bucket) {
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
            if constexpr (is_single_bucket) {
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

        template <typename K>
        consteval static bool _can_pass_through() noexcept {
            return detail::transparency::_can_pass_through<_Ty, K>();
        }

        template <typename K>
        constexpr static std::pair<_Ty, int> _try_pass_through(const K& x) noexcept {
            return detail::transparency::_try_pass_through<_Ty, _Tu, _Ufirst, _Ulast, K>(x);
        }

        template <typename K>
        consteval static bool _is_nothrow_cmp() noexcept {
            if constexpr (_can_pass_through<K>()) {
                return true;
            } else {
                return noexcept(key_compare{}(std::declval<_Ty>(), std::declval<K>()))
                    && noexcept(key_compare{}(std::declval<K>(), std::declval<_Ty>()))
                    && noexcept(key_compare{}(std::declval<K>(), std::declval<K>()));
            }
        }

    public:
        friend constexpr bool operator ==(const set& lr, const set& rr) noexcept = default;
        friend constexpr std::strong_ordering operator <=>(const set& lr, const set& rr) noexcept = default;
    };

    template <typename _Ty>
    constexpr void swap(set_node_handle<_Ty>& lnh, set_node_handle<_Ty>& rnh) noexcept {
        lnh.swap(rnh);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
    constexpr void swap(set<_Ty, _Rfirst, _Rlast, _Sty>& lr, set<_Ty, _Rfirst, _Rlast, _Sty>& rr) noexcept {
        lr.swap(rr);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty, typename _V>
    constexpr std::size_t erase(set<_Ty, _Rfirst, _Rlast, _Sty>& r, const _V& v) noexcept {
        return r.erase(v);
    }

    template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty, typename _UnaryPred>
    constexpr std::size_t erase_if(set<_Ty, _Rfirst, _Rlast, _Sty>& r, _UnaryPred&& pred) noexcept {
        return r.template flip_if<1>(std::forward<_UnaryPred>(pred));
    }
};

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
inline constexpr bool std::ranges::disable_sized_range<irc::set<_Ty, _Rfirst, _Rlast, _Sty>> = !irc::set<_Ty, _Rfirst, _Rlast, _Sty>::is_single_bucket;

template <typename _Ty, _Ty _Rfirst, _Ty _Rlast, typename _Sty>
struct std::hash<irc::set<_Ty, _Rfirst, _Rlast, _Sty>> {
    constexpr std::size_t operator ()(const irc::set<_Ty, _Rfirst, _Rlast, _Sty>& r) const noexcept {
        return std::hash<typename irc::set<_Ty, _Rfirst, _Rlast, _Sty>::internal_array_type>{}(r.as_array());
    }
};
