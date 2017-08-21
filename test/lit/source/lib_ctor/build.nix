{ stdenv }:

stdenv.mkDerivation {
  name = "lib-ctor";
  src = builtins.filterSource stdenv.lib.cleanSourceFilter ./.;

  installPhase = ''
    make install DESTDIR=$out
  '';
}
