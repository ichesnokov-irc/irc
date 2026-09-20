# Integer Range Containers (irc)

**Integer Range Containers (irc)** is a lightweight, portable C++20 header-only zero-dependency library providing high-performance, drop-in replacements for standard containers when working with fixed integer ranges and `enum class` keys. The primary goal is significantly better performance and a vastly reduced memory footprint.

Powered by the `<bit>` header and modern compiler intrinsics, it fuses the APIs of `std::set`, `std::unordered_set`, and `std::bitset` into a single, compile-time ready, CPU-cache-friendly structure.

##  Development Status

The `irc` library is currently undergoing active evaluation and development. Work is actively underway to expand the container ecosystem: `irc::map`, `irc::multimap`, and `irc::multiset` are in development and will be available in future releases. The `irc::set` container is fully complete and ready for production use right now.

---

## 🗺️ Table of Contents

- [Quick Overview](#quick-overview-tldr)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [Iterator & Reference Guarantees](#iterator--reference-guarantees)
- [Performance & Semantic Caveats](#performance--semantic-caveats)
- [Template Architecture & Aliases](#template-architecture--aliases)

---

## ⚡ Quick Overview

| Metric | std::set | std::unordered_set | std::bitset | irc::basic_set (Ours) |
| :--- | :---: | :---: | :---: | :---: |
| **Lookup / Insertion** | O(log N) | O(1) (amortized) | O(1) | **O(1) (strict constant)** |
| **Heap Allocation** | Yes (per node) | Yes (on buckets) | No (stack) | **No (always stack-allocated)** |
| **Iterators Support** | Yes | Yes | No | **Yes (Bidirectional)** |
| **constexpr Support**| Partial (C++20) | No | Partial | **Full (constexpr everywhere)** |

---

## ✨ Key Features

- **Hardware-Accelerated Performance:** Utilizes hardware instructions via <bit> primitives (`std::countr_zero`, `std::popcount`) for ultra-fast bitwise manipulation and iteration.

- **Fully constexpr Implementation:** The entire library - including insertion, lookup, deletion, traversal, and bitwise operations - is fully usable at compile-time.

- **Zero Allocation Footprint:** Internally backed by a fixed-size `std::array`, ensuring stack-allocation safety and maximum CPU cache locality.

- **Universal Hybrid Interface:** Unifies three distinct paradigms into a single API, implementing methods from:
  
  - `std::set` (all methods).
  - `std::unordered_set` (all methods).
  - `std::bitset` (all methods; direct bit manipulation like `&, |, ~, .test(), .set()`).
  
  All methods of the above standard containers are implemented, behaving exactly the same.
  
- **Native Enum Support:** Native handling of strongly-typed `enum class` keys without mandatory manual casting.

- **Drop-In Safe Refactoring:** Implements API stubs for non-relevant methods (e.g., `reserve(), rehash()`) to guarantee a seamless drop-in replacement in legacy codebases without compilation errors.

- **Fixed Transparent Sorting:** Hardcoded to always use `std::less<>` internally for transparency.


---

## 🚀 Quick Start

Since the library is completely header-only, simply copy the irc folder into your project's include directory.

```cpp
#include <iostream>
#include "irc/irc.hpp"

enum class Status : uint8_t { Ready, Pending, Active, Error, Count };

int main() {
    // 1. Compile-time range container usage (constexpr)
    constexpr irc::set<1, 10> compile_time_set = []() {
        irc::set<1, 10> s;
        s.insert(3);
        s.insert(7);
        return s;
    }();
    static_assert(compile_time_set.contains(3));

    // 2. Using with an Enum Class (automatic boundary calculation)
    irc::enum_set<Status> active_statuses;
    active_statuses.insert(Status::Ready);
    active_statuses.insert(Status::Active);

    // Ultra-fast O(1) lookup
    if (active_statuses.contains(Status::Ready)) {
        std::cout << "Status is ready!\n";
    }

    // 3. Bitwise operations like std::bitset
    irc::set<0, 5> set_a{1, 2, 3};
    irc::set<0, 5> set_b{3, 4, 5};
    
    auto intersection = set_a & set_b; // Contains only {3}
    
    // High-performance element traversal via internal loop
    intersection.for_every<1>([](int val) {
        std::cout << "Intersection element: " << val << "\n";
    });

    return 0;
}
```

---

## 🔒 Iterator & Reference Guarantees

> [!NOTE]
> The architecture of `irc::basic_set` provides an exceptional level of reference safety that surpasses standard dynamic containers.

- **Bidirectional by Nature:** Iterators fully satisfy the `std::bidirectional_iterator` concept. You can traverse the range forward (++it) and backward (--it) with maximum efficiency.
- **Total Invalidation Safety:** Neither iterators nor references are ever invalidated, even if elements are concurrently added or erased from the container.
- **Persistence After Erasure:** If you hold an iterator pointing to a key, and that specific key is subsequently erased from the set, the iterator remains valid. You can still safely dereference it (it yields the value of the erased key) or move it forward/backward (++it / --it).

---

## ⚠️ Performance & Semantic Caveats

### 1. Performance Overhead on Large Ranges
While the overall performance of irc is significantly faster than `std::set`, care must be taken when querying sizes on vast ranges. Because `irc::basic_set` does not track a live element counter to optimize memory footprint and mutating performance, the `.size()` and `.empty()` methods evaluate bits dynamically with linear time complexity.

### 2. Stack Allocation Safety with Large Ranges

Because `irc::basic_set` is internally backed by a fixed-size `std::array`, its size in bytes scales linearly with the span of the range (1 byte per 8 elements). The library is tightly optimized for replacing and accelerating a vast number of small-to-medium standard sets. 

However, during generic refactoring, if you find yourself dealing with an uncommonly large integer span that might risk exceeding local thread stack limits, you should safely allocate whole container on the heap. 

### 3. Out-of-Range Element Semantics

The library operates on a strict compile-time defined memory boundary `[First, Last]`. Handling elements outside this specified range depends on which container paradigm you are interfacing with:

* **`std::set`, `std::unordered_set`  Interfaces (Container Methods):**
  * **Erasure is safe (No-Op):** Calling `.erase()` with an out-of-range value is completely valid and naturally results in a safe no-op.
  * **Insertion is Undefined Behavior (UB):** Attempting to `.insert()` an element that falls outside the container's valid range will trigger a runtime `assert()` in debug configurations. In release configurations, this bypasses structural checks and results in **Undefined Behavior (UB)** due to out-of-bounds memory array indexing. Always ensure values are checked within bounds before attempting an insertion.
* **`std::bitset` Interface (Bitwise Methods):**
  * **Exception Generation:** To align with standard expected behaviors, any operations of `std::bitset` interface (such as `.test()`, `.set()`, `.reset()`) with an argument outside the defined `[First, Last]` range **will throw `std::out_of_range` exception**.

### 4. The .size() Method Collision
Fusing the APIs of `std::set` and `std::bitset` introduces a semantic ambiguity regarding `.size()`:
- `std::set::size()` returns the number of elements currently present.
- `std::bitset::size()` returns the total number of bits (capacity).

### 5. Iterators and .swap() and moving
Iterators and references of swapped or moved-in `std::basic_set` objects remain generally valid, but do not change their ownership to new container. They can start point to erased elements.

### 6. Fixed `key_compare` Constraint
> [!IMPORTANT]
> The member types `key_compare` and `value_compare` are permanently aliased to `std::less<>` (making them transparent by default). It is **impossible** to provide a custom comparator or invert the sort order (e.g., using `std::greater`). 

---


## 📐 Template Architecture & Aliases

### The Core Class (`irc::basic_set`)

```cpp
template <
    typename Type, 
    Type First, 
    Type Last, 
    typename StorageType = /* Platform-optimized unsigned integer */
>
class basic_set;
```

### Convenient Type Aliases

| Alias | Description |
| :--- | :--- |
| irc::set<[First], Last> | The primary shorthand. If First is omitted, it automatically defaults to a range of [0..Last]. |
| irc::max_set<Type> | Takes exactly one argument. Maps a closed range spanning from std::numeric_limits<Type>::min() up to std::numeric_limits<Type>::max(). |
| irc::enum_set<Type> | Tailored for `enum class` traversal, automatically calculating bounds based on your underlying enum metrics. |
| irc::bitset<Size> | A drop-in replacement for `std::bitset`, but equipped with all the iterators and interface extensions of `std::set`. |
| irc::bool_set | A built-in alias pre-configured for a closed range of [false, true]. Occupies exactly 1 byte of memory. |

### Enum Range Detection Mechanics

By default, `irc::enum_set` attempts to automatically calculate the logical compile-time range of your enum type. To make this automated detection succeed, your enum definition must include specific marker elements:

- **Upper Bound (Required):** The enum must define either a `count` element (case-insensitive: `count`, `Count`, `COUNT`)  or a `last` element (case-insensitive: `last`, `Last`, `LAST`). If a `count` sentinel is found, the maximum boundary evaluates to `count - 1`. If `last` is provided, it evaluates directly to `last`.
- **Lower Bound (Optional):** You can optionally provide a `first` element (case-insensitive: `first`, `First`, `FIRST`) to establish the lower bound index. If omitted, the lower boundary defaults to `0`.

#### **Usage Examples of `irc::enum_set`**

```cpp
// Example 1: Standard sequential Enum using "Count" sentinel
enum class TaskStatus : uint8_t {
    New,
    InProgress,
    Blocked,
    Done,
    Count // Automatically evaluates range as [New (0), Done (3)]
};

// Example 2: Explicit indexing Enum using custom offsets and "Last" marker
enum class DeviceFlag : int16_t {
    First = 10,
    Read = 10,
    Write = 11,
    Execute = 12,
    Last = 12 // Automatically evaluates range as [First (10), Last (12)]
};

// Compile-time checks validating boundary evaluation properties
static_assert(std::same_as<irc::enum_set<TaskStatus>, irc::basic_set<TaskStatus, TaskStatus::New, TaskStatus::Done>>);
```

#### **Custom Range Specialization**

If your target `enum class` cannot meet these default structural naming constraints (e.g., third-party legacy enums), you can easily override the reflection behavior by providing a custom specialization for the `consteval irc::enum_range` function helper:

```cpp
enum class LegacyColor { Red = 5, Green = 6, Blue = 7 };

// Specialize for enum types that do not have Count/Last/First markers
template <>
consteval std::pair<LegacyColor, LegacyColor> irc::enum_range<LegacyColor>() {
    return {LegacyColor::Red, LegacyColor::Blue};
}

// Now perfectly valid and automatically uses [5, 7] interval
using ColorSet = irc::enum_set<LegacyColor>; 
```

### Alias Mappings

```cpp
enum class Color : int32_t { Red = -1, Green = 0, Blue = 5, Count };

// 1. Integer Mappings
using IntRangeSet       = irc::set<-10, 10>;        // -> irc::basic_set<int, -10, 10>
using UnsignedByte      = irc::set<uint8_t(255)>;   // -> irc::basic_set<uint8_t, 0, 255>
using MaxIntContainer   = irc::max_set<int16_t>;    // -> irc::basic_set<int16_t, -32768, 32767>

// 2. Character Mappings
using AsciiSet          = irc::set<'a', 'z'>;       // -> irc::basic_set<char, 'a', 'z'>

// 3. Enum Mappings
using ColorEnumSet      = irc::enum_set<Color>;     // -> irc::basic_set<Color, Color::Red, Color::Blue>

// 4. Built-in Specializations
using FlagContainer     = irc::bitset<128>;         // -> irc::basic_set<size_t, 0, 127>
using BinaryStateSet    = irc::bool_set;            // -> irc::basic_set<bool, false, true>

// Compile-time checks proving accurate mapping transformations
static_assert(std::same_as<IntRangeSet, irc::basic_set<int, -10, 10>>);
static_assert(std::same_as<MaxIntContainer, irc::basic_set<int16_t, -32768, 32767>>);
static_assert(std::same_as<FlagContainer, irc::basic_set<size_t, 0, 127>>);
static_assert(std::same_as<BinaryStateSet, irc::basic_set<bool, false, true>>);

// Size verification ensuring optimal 1-byte footprint
static_assert(sizeof(BinaryStateSet) == 1);
```

---

## 🛠️ Extended APIs (beyond `std::set`, `std::unordered_set` and `std::bitset`)

The implementation provides several specialized, high-performance features absent from standard containers:

### 1. Set Relationship Predicates
Instead of sequentially iterating and manually evaluating matches, you can instantly compare set configurations using CPU-level word operations:
- `intersects(const basic_set& other)`: Returns `true` if the two sets share at least one common element.
- `is_subset_of(const basic_set& superset)`: Returns `true` if the current set is a subset of the specified superset.
- `is_proper_subset_of(const basic_set& superset)`: Returns `true` if it is a strict subset (the current set is a subset and is not equal to the superset).

### 2. High-Speed Internal Loop Drivers (`for_every` / `rfor_every`)
When performance is paramount, bypass standard iterators entirely. These internal traversal loops allow the compiler to generate heavily optimized block bit-scans and unroll execution loops:
```cpp
const irc::set<1, 5> my_set {2, 4}; // Bits: 0 1 0 1 0
auto print = [](int val) { std::cout << val << " "; };

// 1. Template variants <1> (Fast scan over active elements)
my_set.for_every<1>(print);  // Prints: 2 4 
my_set.rfor_every<1>(print); // Prints: 4 2 

// 2. Template variants <0> (Fast scan over empty slots)
my_set.for_every<0>(print);  // Prints: 1 3 5 
my_set.rfor_every<0>(print); // Prints: 5 3 1 

// 3. Binary variants (using proxy references)
auto sweep = [](int val, auto bit) { 
    if (bit) { std::cout << val << "+ "; } else { std::cout << val << "- "; }; 
};
my_set.for_every(sweep);  // Prints: 1- 2+ 3- 4+ 5-
my_set.rfor_every(sweep); // Prints: 5- 4+ 3- 2+ 1-
```
The Traversal Concepts: The for_every<1> and for_every<0> versions skip storage blocks entirely using processor-native bit manipulation scans. Conversely, the binary callback variants iterate through every possible value to grant you an interactive proxy reference interface (const_reference).

### 3. Block Range Assignments and Predicates (`fill` / `flip` / `flip_if`)
- `fill<bool_value>(from, to)`,`flip(from, to)`: Instantly sets, clears, or flips an entire contiguous sub-interval of elements using a bitwise mask block, bypassing sequential `insert`/`erase` loops.
- `flip_if<bool_value>(predicate)`: Inverts the state of elements that match the provided unary predicate condition. Performance Optimization of `flip_if`: calling `flip_if<1>` selectively skips checking bit positions where elements are absent, providing a drastic speed boost compared to manual conditional loops.
```cpp
irc::set<1, 10> s; // Mapped range: [1..10]

// 1. fill<bool_value>(from, to) specializations
s.fill<1>(2, 6); // Batch insertion -> Set: {2, 3, 4, 5, 6}
s.fill<0>(4, 5); // Batch erasure    -> Set: {2, 3, 6}

// 2. flip(from, to) method
s.flip(5, 7);    // Inverts block states: 5(0->1), 6(1->0), 7(0->1) -> Set: {2, 3, 5, 7}

// 3. flip_if<bool_value>(unary_predicate) overloads (State-filtered)
s.flip_if<1>([](int val) { return val % 2 != 0; }); // Checks active bits (1) only -> Removes {3, 5, 7}
s.flip_if<0>([](int val) { return val == 4; });     // Checks empty spaces (0) only -> Adds {4}
// Current Set: {2, 4}

// 4. flip_if(binary_predicate) overload (Full range sweep)
s.flip_if([](int val, auto bit) {
    return !bit && val > 8; // Evaluates every index with its proxy-bit reference -> Adds {9, 10}
});
// Final Set: {2, 4, 9, 10}
```

### 4. Data Overrides (`as_array` / `data`)
- `as_array(), data()`: Gives access to underlying array.
> [!WARNING]
> The non-const overrides for `as_array()` and `data()` expose memory buffers directly. If modifications populate bits of the `last()` element of array outside the structural boundaries dictated by `back_mask()`, iterator navigation logic and containment invariants break, throwing the program into Undefined Behavior (UB).

## License

Distributed under the MIT License. See LICENSE for more information.