// Copyright (c) 2026 Igor V. Chesnokov <ichesnokov+irc@gmail.com>
// SPDX-License-Identifier: MIT
// Full license text: https://opensource.org/license/mit


#include <irc/bitset.hpp>

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    using BS = irc::auto_bitset<10, 20>;
    if (BS{}.count() != 0) {
        return 1;
    }
    return Catch::Session().run(argc, argv);
}
