#!/usr/bin/env nix-shell
#! nix-shell -i bash -p llvmPackages_39.clang-unwrapped findutils -Q

PWD=$(dirname $0)
ROOT=$PWD/..

find $ROOT \
  -name "archive-rw" -prune \
  -o \( -iname "*.c" -or -iname "*.cpp" -or -iname "*.h" -or -iname "*.mm" \) -exec \
    clang-format -style=llvm -sort-includes -i {} +
