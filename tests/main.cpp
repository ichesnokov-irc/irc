// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit


#include <irc/bitset.hpp>
#include <irc/set.hpp>

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    using BS = irc::auto_bitset<10, 20, uint16_t>;
    uint16_t bag[22] = {0};
    if (BS{bag, 20}.count() != 0) {
        return 1;
    }
    using AS = irc::set<int, 10, 40>;
    if (AS{}.size() != 0) {
        return 1;
    }
    return Catch::Session().run(argc, argv);
}
