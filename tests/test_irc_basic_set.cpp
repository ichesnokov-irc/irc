// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <irc/basic_set.hpp>
#include <irc/max_set.hpp>

#include <tuple>
#include <concepts>

using TestTypes = std::tuple<
    irc::basic_set<signed char, -10, +10>,
    irc::basic_set<signed short, -100, +100>,
    irc::basic_set<signed int, -1000, +1000, std::uint8_t>,
    irc::basic_set<signed int, -1000, +1000, std::uint16_t>,
    irc::basic_set<signed int, -1000, +1000, std::uint32_t>,
    irc::basic_set<signed int, -1000, +1000, std::uint64_t>,
    irc::basic_set<signed long, -1000, +1000, std::uint8_t>,
    irc::basic_set<signed long, -1000, +1000, std::uint16_t>,
    irc::basic_set<signed long, -1000, +1000, std::uint32_t>,
    irc::basic_set<signed long, -1000, +1000, std::uint64_t>,
    irc::basic_set<signed long long, -1000, +1000, std::uint8_t>,
    irc::basic_set<signed long long, -1000, +1000, std::uint16_t>,
    irc::basic_set<signed long long, -1000, +1000, std::uint32_t>,
    irc::basic_set<signed long long, -1000, +1000, std::uint64_t>,
    irc::basic_set<unsigned char, 10, 30>,
    irc::basic_set<unsigned short, 100, 200>,
    irc::basic_set<unsigned int, 1000, 2000, std::uint8_t>,
    irc::basic_set<unsigned int, 1000, 2000, std::uint16_t>,
    irc::basic_set<unsigned int, 1000, 2000, std::uint32_t>,
    irc::basic_set<unsigned int, 1000, 2000, std::uint64_t>,
    irc::basic_set<unsigned long, 1000, 2000, std::uint8_t>,
    irc::basic_set<unsigned long, 1000, 2000, std::uint16_t>,
    irc::basic_set<unsigned long, 1000, 2000, std::uint32_t>,
    irc::basic_set<unsigned long, 1000, 2000, std::uint64_t>,
    irc::basic_set<unsigned long long, 1000, 2000, std::uint8_t>,
    irc::basic_set<unsigned long long, 1000, 2000, std::uint16_t>,
    irc::basic_set<unsigned long long, 1000, 2000, std::uint32_t>,
    irc::basic_set<unsigned long long, 1000, 2000, std::uint64_t>,
    irc::max_set<signed char>,
    irc::max_set<signed char, std::uint8_t>,
    irc::max_set<unsigned char>,
    irc::max_set<unsigned char, std::uint8_t>,
    irc::max_set<signed short>,
    irc::max_set<unsigned short>,
    irc::basic_set<char8_t, 'a', 'z'>,
    irc::basic_set<char16_t, 'a', 'z'>,
    irc::basic_set<char32_t, 'a', 'z'>,
    irc::basic_set<wchar_t, 'a', 'z'>,
    irc::bool_set
>;

TEMPLATE_LIST_TEST_CASE(
    "Testing irc::basic_set",
    "[template][list]",
    TestTypes
) {
    using Set = TestType;
    using Key = typename Set::key_type;
    using It = typename Set::iterator;
    using ConstIt = typename Set::const_iterator;
    using DerefType = decltype(*std::declval<It>());
    constexpr auto First = Set::first();
    constexpr auto Last = Set::last();

    SECTION("Custom set container matches C++20 standard concepts") {

        SECTION("Static Analysis : Named requirements for std::set compliance") {
            STATIC_REQUIRE(std::regular<Set>);
            STATIC_REQUIRE(std::swappable<Set>);
            STATIC_REQUIRE(std::is_default_constructible_v<Set>);
        }

        SECTION("Container types and definitions") {
            STATIC_REQUIRE(std::same_as<typename Set::key_type, Key>);
            STATIC_REQUIRE(std::same_as<typename Set::value_type, Key>);
            STATIC_REQUIRE(std::is_signed_v<typename Set::difference_type>);
            STATIC_REQUIRE(std::is_unsigned_v<typename Set::size_type>);
            STATIC_REQUIRE(std::equivalence_relation<typename Set::key_compare, Key, Key>);
        }

        SECTION("C++20 Ranges concepts for Container") {
            STATIC_REQUIRE(std::ranges::range<Set>);
            STATIC_REQUIRE(std::ranges::forward_range<Set>);
            STATIC_REQUIRE(std::ranges::bidirectional_range<Set>);
            STATIC_REQUIRE_FALSE(std::ranges::random_access_range<Set>);
            STATIC_REQUIRE_FALSE(std::ranges::contiguous_range<Set>);

            STATIC_REQUIRE(std::ranges::common_range<Set>);
            STATIC_REQUIRE_FALSE(std::ranges::borrowed_range<Set>);
        }
    }

    SECTION("Custom set iterators match C++20 iterator concepts") {
        SECTION("Iterator category and concepts compliance") {
            STATIC_REQUIRE(std::bidirectional_iterator<It>);
            STATIC_REQUIRE(std::bidirectional_iterator<ConstIt>);

            STATIC_REQUIRE(std::input_iterator<It>);
            STATIC_REQUIRE(std::input_iterator<ConstIt>);

            STATIC_REQUIRE_FALSE(std::random_access_iterator<It>);
            STATIC_REQUIRE_FALSE(std::contiguous_iterator<It>);
        }

        SECTION("Constant iterator semantics (std::set elements are read-only)") {
            STATIC_REQUIRE(std::same_as<typename std::iterator_traits<It>::value_type, const Key>);
            STATIC_REQUIRE(std::same_as<typename std::iterator_traits<ConstIt>::value_type, const Key>);

            // Ensure that you CANNOT assign a value to the dereferenced iterator
            STATIC_REQUIRE_FALSE(std::is_assignable_v<DerefType, int>);
        }

        SECTION("Reverse Iterator concepts compliance") {
            using RevIt = typename Set::reverse_iterator;
            using ConstRevIt = typename Set::const_reverse_iterator;

            STATIC_REQUIRE(std::bidirectional_iterator<RevIt>);
            STATIC_REQUIRE(std::bidirectional_iterator<ConstRevIt>);
        }
    }
}
