// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <bit>

namespace irc::detail {

    template <std::size_t _Bs>
    consteval auto _n_uint() noexcept {
        static_assert(_Bs <= 8 && std::has_single_bit(_Bs));
        if constexpr (_Bs == 1) {
            return std::uint8_t{};
        } else if constexpr (_Bs == 2) {
            return std::uint16_t{};
        } else if constexpr (_Bs == 4) {
            return std::uint32_t{};
        } else {
            return std::uint64_t{};
        }
    }

    template <std::size_t _Bs>
    using _n_uint_t = decltype(_n_uint<_Bs>());

    template <auto _Max>
    consteval auto _up_uint() noexcept {
        static_assert(_Max < std::numeric_limits<std::uintmax_t>::max(), "Too big range");
        if constexpr (_Max < std::numeric_limits<std::uint8_t>::max()) {
            return std::uint8_t{};
        } else if constexpr (_Max < std::numeric_limits<std::uint16_t>::max()) {
            return std::uint16_t{};
        } else if constexpr (_Max < std::numeric_limits<std::uint32_t>::max()) {
            return std::uint32_t{};
        } else {
            return std::uint64_t{};
        }
    }

    template <auto _Max>
    using _up_uint_t = decltype(_up_uint<_Max>());

    template <typename _Ty>
    consteval auto _underlying_uint() noexcept {
        if constexpr (std::is_enum_v<_Ty>) {
            return _underlying_uint<std::underlying_type_t<_Ty>>();
        } else {
            using _Tu = _n_uint_t<sizeof(_Ty)>;
            return std::conditional_t<std::is_signed_v<_Ty>, std::make_signed_t<_Tu>, _Tu>{};
        }
    }

    template <typename _Ty>
    using _underlying_uint_t = decltype(_underlying_uint<_Ty>());

    template <auto _Max = std::numeric_limits<std::uintmax_t>::max()>
    consteval auto _fast_sty() noexcept {
        if constexpr (sizeof(void*) == 8) {
            if constexpr (_Max < 32) {
                return std::uint_fast32_t{};
            } else {
                return std::uint_fast64_t{};
            }
        } else {
            return _n_uint<sizeof(int)>();
        }
    }

    template <auto _Max = std::numeric_limits<std::uintmax_t>::max()>
    using _fast_sty_t = decltype(_fast_sty<_Max>());

    template <typename _Ty, _Ty _First, _Ty _Last>
    using _fast_sty_for_range_t = _fast_sty_t<
        static_cast<std::uintmax_t>(_Last) - static_cast<std::uintmax_t>(_Last)>;

    template <unsigned int _Radix = 10>
    constexpr unsigned int _ndigits(auto v, bool minus = true, bool plus = false) noexcept {
        static_assert(_Radix >= 2 && _Radix <= 36);
        unsigned int ndig = v != 0 && (v < 0 ? minus : plus);
        do {
            v /= _Radix;
            ++ndig;
        } while (v);
        return ndig;
    }

    template <typename _T, bool onStack = (sizeof(_T) < sizeof(void*) * 1024)>
    class _temp {
        _T tmp;
    public:
        template <typename ..._Args>
        _temp(_Args&& ...args) : tmp{std::forward<_Args>(args)...} {}

        _T* operator ->() noexcept { return &tmp; }
        const _T* operator ->() const noexcept { return &tmp; }
    };

    template <typename _T>
    class _temp<_T, false> {
        _T* const tmp;
    public:
        template <typename ..._Args>
        _temp(_Args&& ...args) : tmp{new _T{std::forward<_Args>(args)...}} {}
        ~_temp() { delete tmp; }

        _T* operator ->() noexcept { return tmp; }
        const _T* operator ->() const noexcept { return tmp; }
    };
}
