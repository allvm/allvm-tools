with import <allvm> {};

let
  AB = import ./build.nix { stdenv = wllvmStdenv; };
  AB-allexe = pkgToAllexe "AB" AB;
in
  AB-allexe
