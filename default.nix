{ nixpkgs ? import ./nix/fetch-nixpkgs.nix }:

with import nixpkgs {};
{
  allvm-tools-gcc = callPackage ./nix/build.nix {
    inherit (llvmPackages_6) llvm clang lld;
  };

  allvm-tools-clang = callPackage ./nix/build.nix {
    inherit (llvmPackages_6) stdenv llvm clang lld;
  };
}
