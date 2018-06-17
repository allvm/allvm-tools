#!/bin/sh

cachix push allvm --watch-store &

nix-build | cachix push allvm
