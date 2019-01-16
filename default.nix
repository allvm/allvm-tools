{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }@args:

let
  release = import ./nix/release.nix args;
in {
  allvm-tools = release.musl.allvm-tools-gcc8;
}
