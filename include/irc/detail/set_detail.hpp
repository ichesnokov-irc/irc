// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <type_traits>

namespace irc::detail {

    template <auto _R1, auto _R2>
    struct _deduce_range {
        static_assert(std::is_same_v<decltype(_R1), decltype(_R2)>, "First and last range values must be of the same type");
        using me = _deduce_range;
        using type = decltype(_R1);
        constexpr static type first = _R1;
        constexpr static type last = _R2;
    };

    template <auto _R1>
    struct _deduce_range<_R1, nullptr> {
        using me = _deduce_range<decltype(_R1){}, _R1>;
    };

    template <auto _R1, auto _R2>
    using _deduce_range_t = typename _deduce_range<_R1, _R2>::me;
}
