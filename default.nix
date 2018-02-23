{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }@args:

let
  release = import ./nix/release.nix args;
in {
  inherit (release.musl) allvm-tools-clang4;
}
