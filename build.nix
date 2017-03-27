# This file is usually 'default.nix'
{ stdenv, cmake, git, llvm, clang, lld, zlib, python2 }:

let
  inherit (stdenv) lib;
  gitrev = lib.commitIdFromGitRepo ./.git;
  gitshort = builtins.substring 0 7 gitrev;

  sourceFilter = name: type: let baseName = baseNameOf (toString name); in
    (lib.cleanSourceFilter name type) && !(
      (type == "directory" && (lib.hasPrefix "build" baseName ||
                               lib.hasPrefix "install" baseName))
  );

in

stdenv.mkDerivation {
  name = "allvm-tools-git-${gitshort}";
  version = gitshort;

  src = builtins.filterSource sourceFilter ./.;

  nativeBuildInputs = [ cmake git python2 ];
  buildInputs = [ llvm lld zlib ];

  doCheck = true;

  cmakeFlags = [
    "-DGITVERSION=${gitshort}-dev"
    "-DCLANGFORMAT=${clang.cc}/bin/clang-format"
  ];

  # Check formatting, not parallel for more readable output
  preCheck = ''
    make check-format -j1
  '';

  postBuild = ''
    paxmark m bin/alley
  '';

  # musl needs stackprotector disabled
  hardeningDisable = "stackprotector";

  enableParallelBuilding = true;
}
