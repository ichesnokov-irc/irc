// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail/enum_range_detail.hpp"

namespace irc {
    template <typename _Enum>
    consteval std::pair<_Enum, _Enum> enum_range() noexcept {
        return detail::_enum_traits<_Enum>::range();
    }
}
