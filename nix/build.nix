# This file is usually 'default.nix'
{ stdenv
, cmake, git, python2
, llvm, clang, lld, zlib
# Only try to build docs on x86/x86_64,
# to avoid haskell dependency elsewhere
# Not only is it rather heavy but not always supported
, buildDocs ? stdenv.hostPlatform.isX86 or false
, pandoc, texlive
, useClangWerrorFlags ? stdenv.cc.isClang
}:

# Make sure no one tries to enable clang-specific flags
# when building using gcc or any non-clang
assert useClangWerrorFlags -> stdenv.cc.isClang;

let
  inherit (stdenv) lib;
  gitrev = if (builtins.pathExists ../.git) then lib.commitIdFromGitRepo ../.git else "0000000000000000000000000000000000000000";
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

  src = builtins.filterSource sourceFilter ./..;

  nativeBuildInputs = [ cmake git python2 ] ++ lib.optionals buildDocs [ pandoc tex ];
  buildInputs = [ llvm lld zlib ];

  outputs = [ "out" /* "dev" */ ] ++ stdenv.lib.optional buildDocs "doc";

  doCheck = true;

  cmakeFlags = [
    "-DBUILD_DOCS=${if buildDocs then "ON" else "OFF"}"
    "-DGITVERSION=${gitshort}-dev"
    "-DCLANGFORMAT=${clang.cc}/bin/clang-format"
  ] ++ stdenv.lib.optional useClangWerrorFlags "-DUSE_CLANG_WERROR_FLAGS=ON";

  # Check formatting, not parallel for more readable output
  preCheck = ''
    make check-format -j1
  '';

  postBuild = ''
    paxmark m bin/alley
  '';

  enableParallelBuilding = true;
}
