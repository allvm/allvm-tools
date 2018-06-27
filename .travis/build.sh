#!/bin/sh

cachix push allvm --watch-store &

nix-build -o result | cachix push allvm

# Push build deps, alt outputs, and ensure paths are pushed if already on allvm.cache.org
nix-store -qR --include-outputs $(nix-store -q --deriver ./result) | cachix push allvm
