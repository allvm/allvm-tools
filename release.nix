let
  pkgs = import <nixpkgs> {};

  jobs = rec {
    allvm-tools = pkgs.callPackage ./build.nix {
      inherit (pkgs.llvmPackages_4) llvm clang lld;
    };
  };
in jobs
