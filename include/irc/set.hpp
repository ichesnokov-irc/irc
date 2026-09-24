// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include "detail/set_detail.hpp"
#include "basic_set.hpp"

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
    using set = basic_set<
        typename detail::_deduce_range_t<_R1, _R2>::item_type,
        detail::_deduce_range_t<_R1, _R2>::first,
        detail::_deduce_range_t<_R1, _R2>::last,
        _Sty
    >;
}
