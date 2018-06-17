#!/bin/sh

nix run -f '<nixpkgs>' git -c git pull --unshallow
