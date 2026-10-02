// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include "basic_set_test_types.hpp"
#include "comparison_test_types.hpp"

namespace {
    template <typename Set, typename Preset>
    void Test() {
        using Type = typename Preset::Type;
        using Key = typename Set::key_type;

        constexpr Type Value = Preset::Value;
        constexpr Key StoredValue = static_cast<Key>(Preset::StoredValue);
        constexpr bool ShouldExist = Preset::ShouldExist;
        constexpr bool LessThanAll = Preset::LessThanAll;

        SECTION("Zero elements - empty container") {
            Set s;

            // Test Set::erase<K>
            auto erased = s.erase(Value);
            REQUIRE(erased == 0);

            // Test Set::find<K>
            auto it = s.find(Value);
            REQUIRE(it == s.end());

            // Test Set::contains<K>
            bool contains = s.contains(Value);
            REQUIRE(!contains);

            // Test Set::count<K>
            auto count = s.count(Value);
            REQUIRE(count == 0);

            // Test Set::lower_bound<K>
            auto it_lb = s.lower_bound(Value);
            REQUIRE(it_lb == s.end());

            // Test Set::upper_bound<K>
            auto it_ub = s.upper_bound(Value);
            REQUIRE(it_ub == s.end());

            // Test Set::equal_range<K>
            auto [er1, er2] = s.equal_range(Value);
            REQUIRE(er1 == s.end());
            REQUIRE(er2 == s.end());
        }

        SECTION("Single element") {
            Set s;
            if (ShouldExist) {
                s.insert(StoredValue);
                REQUIRE(s.size() == 1);
            }

            // Test Set::erase<K>
            auto erased = s.erase(Value);
            REQUIRE((erased == 1) == ShouldExist);

            if (ShouldExist) {
                s.insert(StoredValue);
                REQUIRE(s.size() == 1);
            }

            // Test Set::find<K>
            auto it = s.find(Value);
            if (ShouldExist) {
                REQUIRE(it != s.end());
                REQUIRE(*it == StoredValue);
            } else {
                REQUIRE(it == s.end());
            }

            // Test Set::contains<K>
            bool contains = s.contains(Value);
            REQUIRE(contains == ShouldExist);

            // Test Set::count<K>
            auto count = s.count(Value);
            REQUIRE(count == (ShouldExist ? 1 : 0));

            // Test Set::lower_bound<K>
            auto it_lb = s.lower_bound(Value);
            REQUIRE(it_lb == it);

            // Test Set::upper_bound<K>
            auto it_ub = s.upper_bound(Value);
            if (LessThanAll) {
                REQUIRE(it_ub == s.begin());
            } else {
                REQUIRE(it_ub == s.end());
            }

            // Test Set::equal_range<K>
            auto [er1, er2] = s.equal_range(Value);
            if (ShouldExist) {
                REQUIRE(er1 == it_lb);
                REQUIRE(er2 == it_ub);
                REQUIRE(*er1 == StoredValue);
                REQUIRE(er2 == s.end());
            } else {
                REQUIRE(er1 == er2); // empty range
                if (LessThanAll) {
                    REQUIRE(er1 == s.begin());
                    REQUIRE(er2 == s.begin());
                } else {
                    REQUIRE(er1 == s.end());
                    REQUIRE(er2 == s.end());
                }
            }
        }

        if constexpr (ShouldExist && Preset::Value1 && Preset::StoredValue1) {
            constexpr Type Value1 = *Preset::Value1;
            constexpr Key StoredValue1 = static_cast<Key>(*Preset::StoredValue1);

            SECTION("Adjacent elements - both present") {
                Set s;
                s.insert(StoredValue);
                s.insert(StoredValue1);
                REQUIRE(s.size() == 2);

                // Test Set::erase<K>
                auto erased = s.erase(Value);
                REQUIRE(erased == 1);
                REQUIRE(s.size() == 1);

                s.insert(StoredValue);
                REQUIRE(s.size() == 2);

                // Test Set::find<K>
                auto it = s.find(Value);
                REQUIRE(it != s.end());
                REQUIRE(*it == StoredValue);

                auto it1 = s.find(Value1);
                REQUIRE(it1 != s.end());
                REQUIRE(*it1 == StoredValue1);

                // Test Set::contains<K>
                bool contains = s.contains(Value);
                REQUIRE(contains);
                bool contains1 = s.contains(Value1);
                REQUIRE(contains1);

                // Test Set::count<K>
                auto count = s.count(Value);
                REQUIRE(count == 1);
                auto count1 = s.count(Value1);
                REQUIRE(count1 == 1);

                // Test Set::lower_bound<K>
                auto it_lb = s.lower_bound(Value);
                REQUIRE(it_lb == it);
                auto it_lb1 = s.lower_bound(Value1);
                REQUIRE(it_lb1 == it1);

                // Test Set::upper_bound<K>
                auto it_ub = s.upper_bound(Value);
                REQUIRE(it_ub == it1);
                auto it_ub1 = s.upper_bound(Value1);
                REQUIRE(it_ub1 == s.end());

                // Test Set::equal_range<K>
                auto [er1, er2] = s.equal_range(Value);
                REQUIRE(er1 == it);
                REQUIRE(*er1 == StoredValue);
                REQUIRE(er2 == it1);
                REQUIRE(*er2 == StoredValue1);

                auto [er11, er21] = s.equal_range(Value1);
                REQUIRE(er11 == it1);
                REQUIRE(*er11 == StoredValue1);
                REQUIRE(er21 == s.end());
            }

            SECTION("Adjacent elements - first present") {
                Set s;
                s.insert(StoredValue);
                REQUIRE(s.size() == 1);

                // Test Set::erase<K>
                auto erased = s.erase(Value);
                REQUIRE(erased == 1);
                REQUIRE(s.size() == 0);

                s.insert(StoredValue);
                REQUIRE(s.size() == 1);

                // Test Set::find<K>
                auto it = s.find(Value);
                REQUIRE(it != s.end());
                REQUIRE(*it == StoredValue);

                auto it1 = s.find(Value1);
                REQUIRE(it1 == s.end());

                // Test Set::contains<K>
                bool contains = s.contains(Value);
                REQUIRE(contains);
                bool contains1 = s.contains(Value1);
                REQUIRE(!contains1);

                // Test Set::count<K>
                auto count = s.count(Value);
                REQUIRE(count == 1);
                auto count1 = s.count(Value1);
                REQUIRE(count1 == 0);

                // Test Set::lower_bound<K>
                auto it_lb = s.lower_bound(Value);
                REQUIRE(it_lb == it);
                auto it_lb1 = s.lower_bound(Value1);
                REQUIRE(it_lb1 == s.end());

                // Test Set::upper_bound<K>
                auto it_ub = s.upper_bound(Value);
                REQUIRE(it_ub == s.end());
                auto it_ub1 = s.upper_bound(Value1);
                REQUIRE(it_ub1 == s.end());

                // Test Set::equal_range<K>
                auto [er1, er2] = s.equal_range(Value);
                REQUIRE(er1 == it);
                REQUIRE(*er1 == StoredValue);
                REQUIRE(er2 == s.end());

                auto [er11, er21] = s.equal_range(Value1);
                REQUIRE(er11 == s.end());
                REQUIRE(er21 == s.end());
            }

            SECTION("Adjacent elements - second present") {
                Set s;
                s.insert(StoredValue1);
                REQUIRE(s.size() == 1);

                // Test Set::erase<K>
                auto erased = s.erase(Value1);
                REQUIRE(erased == 1);
                REQUIRE(s.size() == 0);

                s.insert(StoredValue1);
                REQUIRE(s.size() == 1);

                // Test Set::find<K>
                auto it = s.find(Value);
                REQUIRE(it == s.end());

                auto it1 = s.find(Value1);
                REQUIRE(it1 != s.end());
                REQUIRE(*it1 == StoredValue1);

                // Test Set::contains<K>
                bool contains = s.contains(Value);
                REQUIRE(!contains);
                bool contains1 = s.contains(Value1);
                REQUIRE(contains1);

                // Test Set::count<K>
                auto count = s.count(Value);
                REQUIRE(count == 0);
                auto count1 = s.count(Value1);
                REQUIRE(count1 == 1);

                // Test Set::lower_bound<K>
                auto it_lb = s.lower_bound(Value);
                REQUIRE(it_lb == it1);
                auto it_lb1 = s.lower_bound(Value1);
                REQUIRE(it_lb1 == it1);

                // Test Set::upper_bound<K>
                auto it_ub = s.upper_bound(Value);
                REQUIRE(it_ub == it1);
                auto it_ub1 = s.upper_bound(Value1);
                REQUIRE(it_ub1 == s.end());

                // Test Set::equal_range<K>
                auto [er1, er2] = s.equal_range(Value);
                REQUIRE(er1 == it1);
                REQUIRE(*er1 == StoredValue1);
                REQUIRE(er2 == it1);
                REQUIRE(*er2 == StoredValue1);

                auto [er11, er21] = s.equal_range(Value1);
                REQUIRE(er11 == it1);
                REQUIRE(*er11 == StoredValue1);
                REQUIRE(er21 == s.end());
            }
        }
    };
}

TEMPLATE_LIST_TEST_CASE(
    "Testing irc::basic_set transparency-aware methods",
    "[template]",
    BasicSetTestTypes
) {
    using Set = TestType;
    using Key = typename Set::key_type;

    constexpr Key First = Set::first();
    constexpr Key Last = Set::last();
    constexpr Key Mid = First + static_cast<Key>((Last - First) / 2);

    using Presets = ComparisonPresets<Key, First, Mid, Last>;

    std::apply([](auto&&... args) {
        (Test<Set, std::remove_cvref_t<decltype(args)>>(), ...);
    }, Presets{});
}
