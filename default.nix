{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }@args:

let
  release = import ./nix/release.nix args;
in
  release.allvm-tools
