// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "basic_set.hpp"

namespace irc {

    template <
        std::size_t _N,
        typename _Sty = detail::_fast_sty_t<_N - 1>
    >
    using bitset = basic_set<
        std::size_t,
        0,
        _N - 1,
        _Sty
    >;
}
