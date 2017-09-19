{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }:

with import nixpkgs {};
{
  allvm-tools-gcc = callPackage ./nix/build.nix {
    inherit (llvmPackages_5) llvm clang lld;
  };

  allvm-tools-clang = callPackage ./nix/build.nix {
    inherit (llvmPackages_5) stdenv llvm clang lld;
  };
}
