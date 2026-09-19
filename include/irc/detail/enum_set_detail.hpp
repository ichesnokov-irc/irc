// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <type_traits>

namespace irc::detail {

    template <typename _Enum>
    class _enum_traits {
        static_assert(std::is_enum_v<_Enum>, "irc::enum_set<> can be used only with enum/enum class type");

        constexpr static bool
            has_count = requires{ _Enum::count; },
            has_Count = requires{ _Enum::Count; },
            has_COUNT = requires{ _Enum::COUNT; },
            has_first = requires{ _Enum::first; },
            has_First = requires{ _Enum::First; },
            has_FIRST = requires{ _Enum::FIRST; },
            has_last =  requires{ _Enum::last; },
            has_Last =  requires{ _Enum::Last; },
            has_LAST =  requires{ _Enum::LAST; };

        static_assert(has_count + has_Count + has_COUNT <= 1,
            "enum may have only one of 'count'/'Count'/'COUNT' enumerators");

        static_assert(has_first + has_First + has_FIRST <= 1,
            "enum may have only one of 'first'/'First'/'FIRST' enumerators");

        static_assert(has_last + has_Last + has_LAST <= 1,
            "enum may have only one of 'last'/'Last'/'LAST' enumerators");

        static_assert((has_count || has_first || has_last) + (has_Count || has_First || has_Last) + (has_COUNT || has_FIRST || has_LAST) <= 1,
            "enum cannot combine case of special enumerators");

        static_assert(has_count + has_Count + has_COUNT + has_last + has_Last + has_LAST == 1,
            "enum must have 'count' ('Count', 'COUNT') or 'last' ('Last', 'LAST') - one and only one of these enumerators");

        consteval static _Enum _prev(_Enum v) noexcept {
            return static_cast<_Enum>(static_cast<std::underlying_type_t<_Enum>>(v) - 1);
        }

    public:
        consteval static _Enum first() noexcept {
            if constexpr (has_first) {
                return _Enum::first;
            } else if constexpr (has_First) {
                return _Enum::First;
            } else if constexpr (has_FIRST) {
                return _Enum::FIRST;
            } else {
                return _Enum{};
            }
        }

        consteval static _Enum last() noexcept {
            if constexpr (has_count) {
                return _prev(_Enum::count);
            } else if constexpr (has_Count) {
                return _prev(_Enum::Count);
            } else if constexpr (has_COUNT) {
                return _prev(_Enum::COUNT);
            } else if constexpr (has_last) {
                return _Enum::last;
            } else if constexpr (has_Last) {
                return _Enum::Last;
            } else if constexpr (has_LAST) {
                return _Enum::LAST;
            } else {
                return _prev(_Enum{});
            }
        }
    };
}
