#!/bin/sh

VERSION=3.9.0

ROOT=$(readlink -f $(dirname $0))
UTIL_DIR=$ROOT/../utils

rm $UTIL_DIR -rf

dir=`mktemp -d`
trap 'rm -rf "$dir"' EXIT

cd $dir

curl http://llvm.org/releases/${VERSION}/llvm-${VERSION}.src.tar.xz | tar xJvf - llvm-${VERSION}.src/utils/lit

mv llvm-${VERSION}.src/utils $UTIL_DIR

# Remove things that only take up space since we don't use them
rm -rf $UTIL_DIR/lit/examples
rm -rf $UTIL_DIR/lit/tests
