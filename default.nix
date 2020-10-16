{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }@args:

let
  release = import ./nix/release.nix args;
in {
  allvm-tools = release.default.allvm-tools-clang4;
}
