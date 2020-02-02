self: super: rec {
  allvm-tools = super.callPackage ./build.nix {
    inherit (self.llvmPackages_5) llvm clang lld;
  };

  allvm-tools-variants = super.recurseIntoAttrs {
    inherit (self) allvm-tools;
  };
}
