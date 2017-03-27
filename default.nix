let
  default_nixpkgs = (import <nixpkgs> {}).fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs-channels";
    rev = "ab1078806ecf7f1ef28b3a0bd7cda1e9af8e7875"; # current 17.03 beta
    sha256 = "09xm5i7bghb8ndihcsvnc1r42inpv6wzwhk6rdl75hpzhr290s18";
  };
in
{ nixpkgs ? default_nixpkgs }:

with import nixpkgs {};
callPackage ./build.nix {
  inherit (llvmPackages_4) llvm clang lld;
}
