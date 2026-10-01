// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <tuple>
#include <type_traits>
#include <limits>
#include <optional>

template <typename _Ty>
struct TransparentlyComparable {
	_Ty v;
	
	constexpr explicit TransparentlyComparable(_Ty v) noexcept : v(v) {}

	constexpr auto operator <=>(_Ty v2) const noexcept {
		return v <=> v2;
	}

	constexpr bool operator ==(_Ty v2) const noexcept {
		return v == v2;
	}
};

template <bool _LessThanAll>
struct TransparentlyIncomparable {
	constexpr TransparentlyIncomparable(...) noexcept {}

	template <typename _Ty>
	friend constexpr bool operator <(TransparentlyIncomparable, _Ty) noexcept {
		return _LessThanAll;
	}

	template <typename _Ty>
	friend constexpr bool operator <(_Ty, TransparentlyIncomparable) noexcept {
		return !_LessThanAll;
	}
};

template <typename KS, typename K>
consteval std::optional<KS> _TryGreater(K k) noexcept {
	if (k < std::numeric_limits<K>::max()) {
		return KS(static_cast<K>(k + 1ll));
	} else {
		return std::nullopt;
	}
}

template <typename _Ty, auto _Value, bool _ShouldExist, bool _LessThanAll>
struct ComparisonPreset {
	using Type = _Ty;
	using StoredType = decltype(_Value);
	
	constexpr static Type Value = Type{_Value};
	constexpr static std::optional<Type> Value1 = _TryGreater<Type>(_Value);

	constexpr static StoredType StoredValue = _Value;
	constexpr static std::optional<StoredType> StoredValue1 = _TryGreater<StoredType>(_Value);

	constexpr static bool ShouldExist = _ShouldExist;
	constexpr static bool LessThanAll = _LessThanAll;
};

template <typename _Ty, _Ty _Rfirst, _Ty _Rmid, _Ty _Rlast>
using ComparisonPresets = std::tuple<
	  ComparisonPreset<TransparentlyComparable<_Ty>, _Rfirst, true, false>
	, ComparisonPreset<TransparentlyComparable<_Ty>, _Rmid, true, false>
	, ComparisonPreset<TransparentlyComparable<_Ty>, _Rlast, true, false>
	, ComparisonPreset<long long, _Rfirst, true, false>
	, ComparisonPreset<long long, _Rmid, true, false>
	, ComparisonPreset<long long, _Rlast, true, false>
	, ComparisonPreset<long long, std::numeric_limits<long long>::min(), false, true>
	, ComparisonPreset<long long, std::numeric_limits<long long>::max(), false, false>
	, ComparisonPreset<unsigned long long, std::numeric_limits<unsigned long long>::max(), false, false>
	, ComparisonPreset<TransparentlyIncomparable<true>, _Ty{}, false, true>
	, ComparisonPreset<TransparentlyIncomparable<false>, _Ty{}, false, false>
>;
