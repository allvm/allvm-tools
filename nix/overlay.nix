self: super: rec {
  allvm-tools = super.callPackage ./build.nix {
    inherit (self.llvmPackages_7) llvm lld;
  };

  allvm-tools-variants = super.recurseIntoAttrs {
    inherit (self) allvm-tools;
  };
}
