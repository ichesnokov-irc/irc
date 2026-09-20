// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <irc/basic_set.hpp>
#include <irc/max_set.hpp>

#include <vector>
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
    irc::basic_set<char8_t, 'a', 'z'>,
    irc::basic_set<char16_t, 'a', 'z'>,
    irc::basic_set<char32_t, 'a', 'z'>,
    irc::basic_set<wchar_t, 'a', 'z'>
>;

TEMPLATE_LIST_TEST_CASE(
    "Testing irc::basic_set",
    "[template]",
    TestTypes
) {
    using Set = TestType;
    using Key = typename Set::key_type;
    using It = typename Set::iterator;
    using ConstIt = typename Set::const_iterator;
    using DerefType = decltype(*std::declval<It>());
    constexpr auto First = Set::first();
    constexpr auto Last = Set::last();
    constexpr auto Mid = Key{Set::first() + (Set::last() - Set::first()) / 2};
    static_assert(First != Last && First != Mid && Last != Mid, "Check boundaries");

    SECTION("Custom set container matches C++20 standard concepts") {

        SECTION("Static Analysis : Named requirements for std::set compliance") {
            STATIC_REQUIRE(std::regular<Set>);
            STATIC_REQUIRE(std::swappable<Set>);
            STATIC_REQUIRE(std::is_default_constructible_v<Set>);
            STATIC_REQUIRE(std::is_copy_constructible_v<Set>);
            STATIC_REQUIRE(std::is_copy_assignable_v<Set>);
            STATIC_REQUIRE(std::is_move_constructible_v<Set>);
            STATIC_REQUIRE(std::is_move_assignable_v<Set>);
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
            STATIC_REQUIRE(std::ranges::sized_range<Set> == Set::single_bucket);

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

            // Implicit conversion capability (iterator -> const_iterator)
            STATIC_REQUIRE(std::is_convertible_v<It, ConstIt>);
        }

        SECTION("Reverse Iterator concepts compliance") {
            using RevIt = typename Set::reverse_iterator;
            using ConstRevIt = typename Set::const_reverse_iterator;

            STATIC_REQUIRE(std::bidirectional_iterator<RevIt>);
            STATIC_REQUIRE(std::bidirectional_iterator<ConstRevIt>);
        }
    }

    SECTION("Constructors") {
        SECTION("Default constructor creates an empty set") {
            Set s;

            REQUIRE(s.empty());
            REQUIRE(s.size() == 0);
        }
        
        SECTION("Initializer list constructor with values in range") {
            // Duplicate values (First and Mid) are included to test deduplication
            Set s = { Mid, First, Last, First, Mid };

            // Size must be 3 because duplicates are discarded
            REQUIRE(s.size() == 3);
            REQUIRE_FALSE(s.empty());

            // Verify elements are sorted: [First, Mid, Last]
            std::vector<Key> expected = { First, Mid, Last };
            std::vector<Key> actual(s.begin(), s.end());

            REQUIRE(actual == expected);
        }

        SECTION("Range constructor from another container") {
            std::vector<Key> vec = { Last, First, Mid, First };
            Set s(vec.begin(), vec.end());

            REQUIRE(s.size() == 3);
            REQUIRE(s.count(First) == 1);
            REQUIRE(s.count(Mid) == 1);
            REQUIRE(s.count(Last) == 1);
        }

        SECTION("Copy constructor performs a deep copy") {
            Set original = { First, Mid, Last };
            Set copy(original);

            REQUIRE(copy.size() == original.size());
            REQUIRE(copy == original);

            // Modifying the copy should not alter the original
            copy.erase(Last);
            REQUIRE(copy.size() == 2);
            REQUIRE(original.size() == 3);
            REQUIRE(original.count(Last) == 1);
        }

        SECTION("Move constructor") {
            Set original = { First, Mid, Last };
            Set moved(std::move(original));

            REQUIRE(moved.size() == 3);
            REQUIRE(moved.count(First) == 1);

            // The moved-from container is left in a valid state (empty)
            REQUIRE(original.empty());
        }

        SECTION("Constructing from an empty range of Key objects") {
            const std::vector<Key> empty_src;

            // Boundary test: First == Last window
            Set my_set(empty_src.begin(), empty_src.end());

            REQUIRE(my_set.empty());
            REQUIRE(my_set.size() == 0);
        }

        SECTION("Minimum boundary limit") {
            std::vector<Key> src{ First };
            Set my_set(src.begin(), src.end());

            REQUIRE(my_set.size() == 1);
            REQUIRE(my_set.contains(First));
        }

        SECTION("Maximum boundary limit") {
            std::vector<Key> src{ Last };
            Set my_set(src.begin(), src.end());

            REQUIRE(my_set.size() == 1);
            REQUIRE(my_set.contains(Last));
        }

        SECTION("Standard values strictly within interval margins") {
            std::vector<Key> src{ Key{First + 1}, Key{Last - 1} };
            Set my_set(src.begin(), src.end());

            REQUIRE(my_set.size() == 2);
            REQUIRE(my_set.contains(Key{First + 1}));
            REQUIRE(my_set.contains(Key{Last - 1}));
        }
    }

    SECTION("Assignment") {
        SECTION("Copy Assignment") {
            Set set1;
            set1.insert(First);
            set1.insert(Mid);

            Set set2;
            set2.insert(Last);

            set2 = set1; // Trigger Copy Assignment Operator

            REQUIRE(set2.size() == 2);

            // Deep copy validation: mutating set1 must not affect set2
            set1.insert(Last);
            REQUIRE(set2.size() == 2);

            // Self-assignment safety check
            set2 = *&set2;
            REQUIRE(set2.size() == 2);
        }

        SECTION("Move Assignment") {
            Set set1;
            set1.insert(First);
            set1.insert(Mid);

            Set set2;
            set2 = std::move(set1); // Trigger Move Assignment Operator (Mid, First, Last should be transferred)

            REQUIRE(set2.size() == 2);
            REQUIRE(set1.size() == 0);

            // Verify data was correctly transferred
            auto it = set2.begin();
            REQUIRE(*it == First);
        }
    }

    SECTION("Iterators") {
        SECTION("Iterator Mechanics with Node Tracking (First, Mid, Last)") {
            SECTION("Bidirectional traversal from boundaries") {
                Set s;
                s.insert(First);
                s.insert(Mid);
                s.insert(Last);

                auto it = s.begin(); // Starts at the minimum element node

                // Forward progression
                REQUIRE(*it == First);
                REQUIRE(*(++it) == Mid); // pre-increment
                REQUIRE(*(it++) == Mid); // post-increment
                REQUIRE(*it == Last);

                // Incrementing from the maximum element reaches the past-the-end marker
                REQUIRE(++it == s.end());

                // Backward progression starting right before Last
                REQUIRE(*(--it) == Last); // pre-decrement
                REQUIRE(*(it--) == Last); // post-decrement
                REQUIRE(*it == Mid);
                REQUIRE(*(--it) == First);
                REQUIRE(it == s.begin());
            }

            SECTION("Empty container state tracking") {
                Set empty_set;
                // In an empty set, First and Last variables usually point to the same sentinel
                REQUIRE(empty_set.begin() == empty_set.end());
            }

            SECTION("Const correctness constraints") {
                Set s_mod;
                s_mod.insert(Mid);

                const Set& const_s = s_mod;
                auto c_it = const_s.begin();

                // Ensure begin() on a const container correctly resolves to a const_iterator
                STATIC_REQUIRE(std::is_same_v<decltype(c_it), typename Set::const_iterator>);

                REQUIRE(*c_it == Mid);
                REQUIRE(++c_it == const_s.end());
            }

            SECTION("Single element iteration") {
                Set ss;
                ss.insert(Mid);

                auto it = ss.begin();
                REQUIRE(it != ss.end());
                REQUIRE(*it == Mid);

                // Advancing from a single element must reach the end
                ++it;
                REQUIRE(it == ss.end());

                // Going backward from Last must return to the element
                --it;
                REQUIRE(it == ss.begin());
                REQUIRE(*it == Mid);
            }

            SECTION("Set Iteration: Full Tree Traversal and Sorting") {
                Set s;
                std::vector<Key> inputs = { First, Mid, Last };
                for (Key val : inputs) {
                    s.insert(val);
                }

                // Standard expected sorted order
                std::vector<Key> expected = { First, Mid, Last };

                SECTION("Forward iteration visits all elements in sorted order") {
                    std::vector<Key> iterated_values;
                    for (auto it = s.begin(); it != s.end(); ++it) {
                        iterated_values.push_back(*it);
                    }
                    REQUIRE(iterated_values == expected);
                }

                SECTION("Backward iteration from end() visits elements in reverse order") {
                    std::vector<Key> iterated_values;
                    auto it = s.end();

                    while (it != s.begin()) {
                        --it;
                        iterated_values.push_back(*it);
                    }

                    std::vector<Key> expected_reverse = expected;
                    std::reverse(expected_reverse.begin(), expected_reverse.end());

                    REQUIRE(iterated_values == expected_reverse);
                }
            }

            SECTION("Set Iteration: Pre-increment vs Post-increment Mechanics") {
                Set s;
                s.insert(First);
                s.insert(Mid);
                s.insert(Last);

                SECTION("Pre-increment returns the updated iterator state") {
                    auto it = s.begin();
                    auto advanced = ++it;

                    REQUIRE(*it == Mid);
                    REQUIRE(*advanced == Mid);
                    REQUIRE(it == advanced);
                }

                SECTION("Post-increment returns the original iterator state before advance") {
                    auto it = s.begin();
                    auto original = it++;

                    REQUIRE(*it == Mid);
                    REQUIRE(*original == First);
                    REQUIRE(it != original);
                }
            }

            SECTION("Set Iteration: Standard Algorithms Compatibility") {
                Set s;
                s.insert(First);
                s.insert(Mid);
                s.insert(Last);

                SECTION("Distance calculation matches size via std::distance") {
                    // Requires std::input_or_output_iterator concept to be satisfied
                    auto dist = std::distance(s.begin(), s.end());
                    REQUIRE(dist == 3);
                }

                SECTION("Find value using std::find_if") {
                    auto it = std::find_if(s.begin(), s.end(), [](Key val) {
                        return val == Mid;
                    });

                    REQUIRE(it != s.end());
                    REQUIRE(*it == Mid);
                }
            }

            SECTION("Iterator remains valid after inserting an element outside its path") {
                Set s;
                s.insert(First);
                s.insert(Last);

                auto it = s.begin();
                REQUIRE(*it == First);

                s.insert(Mid); // Inserting intermediate values shouldn't invalidate independent iterators

                // Advancing from First should now cleanly find the Mid
                ++it;
                REQUIRE(*it == Mid);
            }
        }
    }
}
