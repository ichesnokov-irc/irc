// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <set>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <memory>
#include <tuple>
#include <irc/irc.hpp>

// ============================================================================
// Functional Unit Tests
// ============================================================================
using set1 = std::set<int>;
using set2 = std::unordered_set<int>;
using set3 = irc::set<1000>;
using sets = std::tuple<set1, set2, set3>;

TEMPLATE_LIST_TEST_CASE("Set operations with integers 0..1000", "[set][unit]", sets) {

    TestType test_set;

    SECTION("Initial state metrics") {
        REQUIRE(test_set.empty());
        REQUIRE(test_set.size() == 0);
    }

    SECTION("Insertion behavior") {
        REQUIRE(test_set.insert(42).second);
        REQUIRE(test_set.insert(1000).second); // Upper boundary
        REQUIRE(test_set.insert(0).second);    // Lower boundary
        REQUIRE(test_set.size() == 3);

        // Duplicate insertion handling
        auto [iter, inserted] = test_set.insert(42);
        REQUIRE_FALSE(inserted);

        REQUIRE(test_set.size() == 3);
    }

    SECTION("Lookup capabilities") {
        test_set.insert(500);
        test_set.insert(777);

        REQUIRE(test_set.find(500) != test_set.end());
        REQUIRE(test_set.find(999) == test_set.end());
    }

    SECTION("Erasure behavior") {
        test_set.insert(256);
        REQUIRE(test_set.size() == 1);

        REQUIRE(test_set.erase(256));
        REQUIRE(test_set.size() == 0);
        REQUIRE(test_set.empty());

        REQUIRE_FALSE(test_set.erase(256));
    }

    SECTION("Clearing containers") {
        test_set.insert(1);
        test_set.insert(2);
        test_set.clear();

        REQUIRE(test_set.empty());
        REQUIRE(test_set.size() == 0);
    }
}

// ============================================================================
// Performance Benchmarks
// ============================================================================
TEST_CASE("Comparative Performance Benchmarks (Integers 0..1000)", "[set][benchmark]") {
    // Dataset: All even numbers from 0 to 1000
    std::vector<int> dataset;
    for (int i = 0; i <= 1000; i += 2) {
        dataset.push_back(i);
    }

    // Queries: All numbers 0 to 1000 (creates a steady 50% hit / 50% miss ratio)
    std::vector<int> lookup_queries;
    for (int i = 0; i <= 1000; ++i) {
        lookup_queries.push_back(i);
    }

    // --- INSERTION BENCHMARKS ---
    BENCHMARK("std::set Insertion") {
        std::set<int> s;
        for (int val : dataset) s.insert(val);
        return s;
    };

    BENCHMARK("std::unordered_set Insertion") {
        std::unordered_set<int> us;
        for (int val : dataset) us.insert(val);
        return us;
    };

    BENCHMARK("irc::set Insertion") {
        irc::set<1000> is;
        for (int val : dataset) is.insert(val);
        return is;
    };

    // --- LOOKUP BENCHMARKS ---
    std::set<int> target_s(dataset.begin(), dataset.end());
    BENCHMARK("std::set Lookup (Mixed)") {
        size_t hits = 0;
        for (int query : lookup_queries) {
            if (target_s.find(query) != target_s.end()) hits++;
        }
        return hits;
    };

    std::unordered_set<int> target_us(dataset.begin(), dataset.end());
    BENCHMARK("std::unordered_set Lookup (Mixed)") {
        size_t hits = 0;
        for (int query : lookup_queries) {
            if (target_us.find(query) != target_us.end()) hits++;
        }
        return hits;
    };

    irc::set<1000> target_is;
    for (int val : dataset) target_is.insert(val);
    BENCHMARK("irc::set Lookup (Mixed)") {
        size_t hits = 0;
        for (int query : lookup_queries) {
            if (target_is.contains(query)) hits++;
        }
        return hits;
    };
}

int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
