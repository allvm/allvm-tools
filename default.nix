{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }:

with import nixpkgs {};
{
  allvm-tools-gcc = callPackage ./build.nix {
    inherit (llvmPackages_4) llvm clang lld;
  };

  allvm-tools-clang = callPackage ./build.nix {
    inherit (llvmPackages_4) stdenv llvm clang lld;
  };
}
