# 0. Remove the file
rm $1.tar.gz

# 1. Download your release archive
wget "https://github.com/ichesnokov-irc/irc/archive/refs/tags/$1.tar.gz"

# 2. Get SHA-256 (for Conan's conandata.yml)
shasum -a 256 $1.tar.gz

# 3. Get SHA-512 (for vcpkg's portfile.cmake)
shasum -a 512 $1.tar.gz
