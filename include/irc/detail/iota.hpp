// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <cstdint>
#include <ranges>
#include <compare>
#include <iterator>
#include <utility>
#include <type_traits>

namespace irc::detail {

    template <typename _Ty, typename _Tu, typename _Ti, _Tu _Ufirst>
    class _iota_wrapper {
        _Ti v;

    public:
        using value_type = _Ty;
        using difference_type = std::ptrdiff_t;

        constexpr _iota_wrapper() noexcept = default;
        constexpr explicit _iota_wrapper(_Ti v) noexcept :v{v} {}

        constexpr operator _Ty() const noexcept { return static_cast<_Ty>(v + _Ufirst); }

        constexpr _iota_wrapper& operator ++() noexcept { ++v; return *this; }
        constexpr _iota_wrapper operator ++(int) noexcept { auto tmp = *this; ++v; return tmp; }
        constexpr _iota_wrapper& operator --() noexcept { --v; return *this; }
        constexpr _iota_wrapper operator --(int) noexcept { auto tmp = *this; --v; return tmp; }

        constexpr _iota_wrapper& operator +=(difference_type n) noexcept { v = static_cast<_Ti>(v + n); return *this; }
        constexpr _iota_wrapper& operator -=(difference_type n) noexcept { v = static_cast<_Ti>(v - n); return *this; }

        friend constexpr _iota_wrapper operator +(_iota_wrapper w, difference_type n) noexcept { w += n; return w; }
        friend constexpr _iota_wrapper operator +(difference_type n, _iota_wrapper w) noexcept { w += n; return w; }
        friend constexpr _iota_wrapper operator -(_iota_wrapper w, difference_type n) noexcept { w -= n; return w; }

        constexpr difference_type operator -(const _iota_wrapper& other) const noexcept {
            return static_cast<difference_type>(v) - static_cast<difference_type>(other.v);
        }

        constexpr auto operator <=>(const _iota_wrapper& other) const noexcept = default;
    };

    template <typename _Ty, typename _Tu, typename _Ti, _Tu _Ufirst, _Ti _Range>
    consteval auto _make_iota_wrapper() noexcept {
        constexpr auto PastLast = static_cast<_Ti>(_Ufirst) + _Range;
        if constexpr (std::is_integral_v<_Ty> && std::in_range<_Tu>(PastLast)) {
            return std::views::iota(static_cast<_Ty>(_Ufirst), static_cast<_Ty>(PastLast));
        } else {
            using wrapper = _iota_wrapper<_Ty, _Tu, _Ti, _Ufirst>;
            auto view = std::views::iota(wrapper{_Ti{}}, wrapper{_Range});
            static_assert(std::ranges::random_access_range<decltype(view)>, "Ensure that iota view is random access");
            static_assert(std::ranges::sized_range<decltype(view)>, "Ensure that iota view is sized range");
            return view;
        }
    }
}
