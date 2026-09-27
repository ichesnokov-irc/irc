// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail/enum_set_detail.hpp"
#include "basic_set.hpp"

namespace irc {

    template <typename _Enum>
    consteval std::pair<_Enum, _Enum> enum_range() noexcept {
        return detail::_enum_traits<_Enum>::range();
    }

    template <
        typename _Enum,
        typename _Sty = detail::_fast_sty_for_range_t<
            _Enum,
            enum_range<_Enum>().first,
            enum_range<_Enum>().second
        >
    >
    using enum_set = basic_set<
        _Enum,
        enum_range<_Enum>().first,
        enum_range<_Enum>().second,
        _Sty
    >;
}
