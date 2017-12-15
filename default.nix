{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }:

with import nixpkgs {};
{
  allvm-tools-gcc = callPackage ./nix/build.nix {
    inherit (llvmPackages_4) llvm clang lld;
  };

  allvm-tools-clang = callPackage ./nix/build.nix {
    inherit (llvmPackages_4) stdenv llvm clang lld;
  };
}
