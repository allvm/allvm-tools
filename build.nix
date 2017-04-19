# This file is usually 'default.nix'
{ stdenv, cmake, git, llvm, clang, lld, zlib, python2, pandoc, texlive }:

let
  inherit (stdenv) lib;
  gitrev = lib.commitIdFromGitRepo ./.git;
  gitshort = builtins.substring 0 7 gitrev;

  sourceFilter = name: type: let baseName = baseNameOf (toString name); in
    (lib.cleanSourceFilter name type) && !(
      (type == "directory" && (lib.hasPrefix "build" baseName ||
                               lib.hasPrefix "install" baseName))
  );

  tex = texlive.combined.scheme-medium;
in

stdenv.mkDerivation {
  name = "allvm-tools-git-${gitshort}";
  version = gitshort;

  src = builtins.filterSource sourceFilter ./.;

  nativeBuildInputs = [ cmake git python2 pandoc tex ];
  buildInputs = [ llvm lld zlib ];

  doCheck = true;

  cmakeFlags = [
    "-DGITVERSION=${gitshort}-dev"
    "-DCLANGFORMAT=${clang.cc}/bin/clang-format"
  ] ++ stdenv.lib.optional stdenv.cc.isClang "-DUSE_CLANG_WERROR_FLAGS=ON";

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
