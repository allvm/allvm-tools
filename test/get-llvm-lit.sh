#!/usr/bin/env nix-shell
#! nix-shell -i bash -p git coreutils curl gnupatch gnutar xz -Q

set -euo pipefail

REV=477a7cd43583aa11f51daa1f4605cc14c3f5a99f

ROOT=$(readlink -f $(dirname $0))
LIT_DIR=$ROOT/../third_party/lit

rm $LIT_DIR -rf

dir=`mktemp -d`
trap 'rm -rf "$dir"' EXIT

cd $dir

curl -L https://github.com/llvm-mirror/llvm/archive/${REV}.tar.gz | tar xzvf - --wildcards "llvm-*/utils/lit"

POSIXLY_CORRECT=1 patch -p3 -i $ROOT/clean-output-directory.patch -d llvm-*/utils/lit

mv llvm-*/utils/lit $LIT_DIR

# Remove things that only take up space since we don't use them
rm -rf $LIT_DIR/{examples,tests,lit/ExampleTests.*}

cd $LIT_DIR
git rm -rf --cached .
git add -f .

git commit -av -m "lit: Update to (patched) copy from LLVM ${REV}"
