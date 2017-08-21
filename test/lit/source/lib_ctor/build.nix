{ stdenv }:

stdenv.mkDerivation {
  name = "lib-ctor";
  src = builtins.filterSource stdenv.lib.cleanSourceFilter ./.;

  makeFlags = [ "DESTDIR=$(out)" ];
}
