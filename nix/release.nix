{ nixpkgs ? import ./fetch-nixpkgs.nix }:

let
  # Stack of overlays for building allvm-tools
  # Each overlay after the "base" one creates one or more variants and adds
  # them to "allvm-tools-variants" attribute.
  # This is done to try building using a variety of compilers.
  # Dependencies are the same and are built using default stdenv,
  # only the derivation for allvm-tools itself is built differently.

  # Default overlay: introduce allvm-tools top-level
  # This is a reasonable overlay you could symlink to ~/.config/nix/overlays/allvm-tools.nix if you wanted :)
  overlay = import ./overlay.nix;

  # overlay "generators":
  # They gather new variants in "allvm-tools-variants" which is probably not the best
  overlayForLLVMV = llvmVersion:
    self: super: {
      allvm-tools-variants = (super.allvm-tools-variants or {}) // {
        "allvm-tools-clang${llvmVersion}" = super.allvm-tools.override {
          stdenv = self."llvmPackages_${llvmVersion}".stdenv;
        };
        "allvm-tools-clang${llvmVersion}-libcxx" = super.allvm-tools.override {
          stdenv = self."llvmPackages_${llvmVersion}".libcxxStdenv;
        };
      };
    };
  overlayForGCCV = gccVersion:
    self: super: {
      allvm-tools-variants = (super.allvm-tools-variants or {}) // {
        "allvm-tools-gcc-${gccVersion}" = super.allvm-tools.override {
          stdenv = super.overrideCC super.stdenv super."gcc${gccVersion}";
        };
      };
    };

  # Create the stack of overlays:
  overlays = [ overlay ]
    ++ (map overlayForLLVMV [ "4" "5" ])
    ++ (map overlayForGCCV [ "5" "6" "7" ]);

  # Import the package set using our stack of overlays,
  # pull out the allvm-tools-variant attribute set
  pkgsFun = nixpkgsArgs: import nixpkgs ({ inherit overlays; } // nixpkgsArgs);
  getALLVMPkgs = nixpkgsArgs: (pkgsFun nixpkgsArgs).allvm-tools-variants;

in {
  default = getALLVMPkgs {};
  musl = getALLVMPkgs {
    localSystem = { config = "x86_64-unknown-linux-musl"; };
  };
}
