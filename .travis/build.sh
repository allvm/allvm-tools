#!/bin/sh

nix-build -o result | cachix push allvm

# Push build deps, alt outputs, and ensure paths are pushed if already on allvm.cache.org
PATHS="$(nix-store -qR --include-outputs $(nix-store -q --deriver ./result))"

echo "Fetching all build deps..."
nix-store -r $PATHS

echo "Pushing all build deps explicitly..."
echo $PATHS | cachix push allvm
