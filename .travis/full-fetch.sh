#!/bin/sh

# builtins.fetchGit doesn't work with shallow clones, so fetch full
git pull --unshallow
