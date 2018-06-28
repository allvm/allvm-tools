#!/bin/sh

cachix push allvm --watch-store &

nix-build -o result | cachix push allvm

cachix push allvm ./result
