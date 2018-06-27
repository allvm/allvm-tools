# This file is usually 'default.nix'
{ stdenv
, cmake, git, python2
, llvm, clang, lld, zlib
# Only try to build docs on x86/x86_64,
# to avoid haskell dependency elsewhere
# Not only is it rather heavy but not always supported
, buildDocs ? stdenv.hostPlatform.isx86 && !stdenv.hostPlatform.isMusl
# Whoops, our tests attempt to execute x86_64 allexe's,
# which of course doesn't work on other platforms.
# Only run tests on x86 (and non-cross, handled by default automagic)
, doCheck ? stdenv.hostPlatform.isx86
, pandoc, texlive
, useClangWerrorFlags ? stdenv.cc.isClang
}:

# Make sure no one tries to enable clang-specific flags
# when building using gcc or any non-clang
assert useClangWerrorFlags -> stdenv.cc.isClang;

let
  inherit (stdenv) lib;

  src = if (builtins.pathExists ../.git) then builtins.fetchGit ./..
        else { outPath = ./..; revCount = 1234; shortRev = "abcdefgh"; };
  gitshort = src.shortRev;

  tex = texlive.combined.scheme-medium;
in

stdenv.mkDerivation {
  name = "allvm-tools-git-${gitshort}";
  version = gitshort;

  inherit src;

  nativeBuildInputs = [ cmake git python2 ] ++ lib.optionals buildDocs [ pandoc tex ];
  buildInputs = [ llvm lld zlib ];

  outputs = [ "out" /* "dev" */ ] ++ stdenv.lib.optional buildDocs "doc";

  inherit doCheck;

  preConfigure = let s = ./..; in ''
    find ${s}
    git -C ${s} diff-index HEAD --
    cd ${s}
    echo "status: "
    git status
    echo "-------"
    git rev-parse HEAD
    echo "-------"
    cd -
    exit 1
  '';

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
