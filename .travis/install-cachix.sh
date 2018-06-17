#!/bin/sh

nix-env -if https://github.com/cachix/cachix/tarball/master \
  --extra-substituters https://cachix.cachix.org \
  --trusted-public-keys 'cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY= cachix.cachix.org-1:eWNHQldwUO7G2VkjpnjDbWwy4KQ/HNxht7H4SSoMckM='
