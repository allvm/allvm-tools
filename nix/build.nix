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
  # If fetchGit gives us all 0's, try reading from '.git' directly.
  # Not entirely sure why but this seems to happen with Hydra's git inputs.
  gitshort = if src.shortRev != "0000000" then src.shortRev
             else assert builtins.pathExists ../.git; builtins.substring 0 7 (lib.commitIdFromGitRepo ../.git);

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

  cmakeFlags = [
    "-DBUILD_DOCS=${if buildDocs then "ON" else "OFF"}"
    "-DGITVERSION=${gitshort}-dev"
    "-DCLANGFORMAT=${clang.cc}/bin/clang-format"
  ] ++ stdenv.lib.optional useClangWerrorFlags "-DUSE_CLANG_WERROR_FLAGS=ON";

  hardeningDisable = [ "all" ];

  NIX_CFLAGS_COMPILE = [
    #"-Wall" "-Wextra"
    "-O1"
    "-g3"
    "-fsanitize=address"
  ];

  # Check formatting, not parallel for more readable output
  #preCheck = ''
  #  make check-format -j1
  #'';

  # if "paxmark" is available, use it
  postBuild = ''
    if command -v paxmark >/dev/null 2>&1; then
      paxmark m bin/alley
    fi
  '';

  enableParallelBuilding = true;
}
