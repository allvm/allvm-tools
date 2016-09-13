#!/usr/bin/env nix-shell
#! nix-shell -i bash -p llvmPackages_39.clang pythonPackages.autopep8 findutils

find . \( -iname "*.c" -or -iname "*.cpp" -or -iname "*.h" -or -iname "*.mm" \) -exec \
  clang-format -i {} +

find . -iname "*.py" -exec \
  autopep8 --in-place --aggressive --aggressive {} + ;
