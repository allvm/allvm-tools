self: super: rec {
  allvm-tools = super.callPackage ./build.nix {
    inherit (self.llvmPackages_4) llvm clang lld;
  };

  allvm-tools-variants = {
    inherit (self) allvm-tools;
  };
}
