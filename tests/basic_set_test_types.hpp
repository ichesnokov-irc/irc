// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#pragma once

#include <irc/basic_set.hpp>
#include <irc/max_set.hpp>

#include <tuple>

using BasicSetTestTypes = std::tuple<
      irc::basic_set<signed char, -10, +10>
    , irc::basic_set<signed short, -100, +100>
    , irc::basic_set<signed int, -1000, +1000, std::uint8_t>
    , irc::basic_set<signed int, -1000, +1000, std::uint16_t>
    , irc::basic_set<signed int, -1000, +1000, std::uint32_t>
    , irc::basic_set<signed int, -1000, +1000, std::uint64_t>
    , irc::basic_set<signed long, -1000, +1000, std::uint8_t>
    , irc::basic_set<signed long, -1000, +1000, std::uint16_t>
    , irc::basic_set<signed long, -1000, +1000, std::uint32_t>
    , irc::basic_set<signed long, -1000, +1000, std::uint64_t>
    , irc::basic_set<signed long long, -1000, +1000, std::uint8_t>
    , irc::basic_set<signed long long, -1000, +1000, std::uint16_t>
    , irc::basic_set<signed long long, -1000, +1000, std::uint32_t>
    , irc::basic_set<signed long long, -1000, +1000, std::uint64_t>
    , irc::basic_set<unsigned char, 10, 30>
    , irc::basic_set<unsigned short, 100, 200>
    , irc::basic_set<unsigned int, 1000, 2000, std::uint8_t>
    , irc::basic_set<unsigned int, 1000, 2000, std::uint16_t>
    , irc::basic_set<unsigned int, 1000, 2000, std::uint32_t>
    , irc::basic_set<unsigned int, 1000, 2000, std::uint64_t>
    , irc::basic_set<unsigned long, 1000, 2000, std::uint8_t>
    , irc::basic_set<unsigned long, 1000, 2000, std::uint16_t>
    , irc::basic_set<unsigned long, 1000, 2000, std::uint32_t>
    , irc::basic_set<unsigned long, 1000, 2000, std::uint64_t>
    , irc::basic_set<unsigned long long, 1000, 2000, std::uint8_t>
    , irc::basic_set<unsigned long long, 1000, 2000, std::uint16_t>
    , irc::basic_set<unsigned long long, 1000, 2000, std::uint32_t>
    , irc::basic_set<unsigned long long, 1000, 2000, std::uint64_t>
    , irc::max_set<signed char>
    , irc::max_set<signed char, std::uint8_t>
    , irc::max_set<unsigned char>
    , irc::max_set<unsigned char, std::uint8_t>
    , irc::basic_set<char8_t, 'a', 'z'>
    , irc::basic_set<char16_t, 'a', 'z'>
    , irc::basic_set<char32_t, 'a', 'z'>
    , irc::basic_set<wchar_t, 'a', 'z'>
    // TODO: Ranges on limits
>;
