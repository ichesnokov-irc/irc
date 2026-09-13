// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "basic_set.hpp"
#include <limits>

namespace irc {

    template <
        typename _IntT,
        typename _Sty = detail::_fast_sty_t<>
    >
    using max_set = basic_set<
        _IntT,
        std::numeric_limits<_IntT>::min(),
        std::numeric_limits<_IntT>::max(),
        _Sty
    >;

    using bool_set = max_set<bool, std::uint8_t>;
}
