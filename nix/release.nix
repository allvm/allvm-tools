{ nixpkgs ? import ./fetch-nixpkgs.nix }:

with import nixpkgs {};
rec {
  allvm-tools-gcc = callPackage ./build.nix {
    inherit (llvmPackages_4) llvm clang lld;
  };

  allvm-tools-clang = callPackage ./build.nix {
    inherit (llvmPackages_4) stdenv llvm clang lld;
  };

  # Default to clang variant
  allvm-tools = allvm-tools-clang;
}

