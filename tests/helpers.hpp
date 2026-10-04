// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <optional>
#include <limits>

template <typename KS, typename K>
consteval std::optional<KS> _TryGreater(K k) noexcept {
	if (k < std::numeric_limits<K>::max()) {
		return KS(static_cast<K>(k + 1ll));
	} else {
		return std::nullopt;
	}
}

template <typename KS, typename K>
consteval std::optional<KS> _TryLess(K k) noexcept {
	if (k > std::numeric_limits<K>::min()) {
		return KS(static_cast<K>(k - 1ll));
	} else {
		return std::nullopt;
	}
}

