{ nixpkgs ? import ./fetch-nixpkgs.nix, nixpkgsArgs ? {} }:

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
  # TODO: build llvmPackages using same stdenv's or it'll rarely work (linking, ABI)
  overlayForLLVMV = llvmVersion:
    self: super: {
      allvm-tools-variants = (super.allvm-tools-variants or {}) // {
        "allvm-tools-clang${llvmVersion}" = super.allvm-tools.override {
          stdenv = self."llvmPackages_${llvmVersion}".stdenv;
        };
        # XXX: Definitely can't switch to libc++ "just" for this
        #"allvm-tools-clang${llvmVersion}-libcxx" = super.allvm-tools.override {
        #  stdenv = self."llvmPackages_${llvmVersion}".libcxxStdenv;
        #};
      };
    };
  overlayForGCCV = gccVersion:
    self: super: {
      allvm-tools-variants = (super.allvm-tools-variants or {}) // {
        "allvm-tools-gcc${gccVersion}" = super.allvm-tools.override {
          stdenv = super.overrideCC super.stdenv super."gcc${gccVersion}";
        };
      };
    };

  # Create the stack of overlays:
  overlays = [ overlay ]
    ++ (map overlayForLLVMV [ "4" "5" "6" "7" ])
    ++ (map overlayForGCCV [ /* "5" "6" */ "7" "8" ])
    ;

  # Import the package set using our stack of overlays,
  # pull out the allvm-tools-variant attribute set
  pkgs = import nixpkgs (nixpkgsArgs // {inherit overlays;});
in {
  default = pkgs.allvm-tools-variants;
  musl = pkgs.pkgsMusl.allvm-tools-variants;
}
