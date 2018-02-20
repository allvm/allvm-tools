let
  default = import ./default.nix {};
  tools = default.allvm-tools-clang;

  format-shell = target: tools.overrideAttrs(o: {

    dontUseCmakeBuildDir = true;

    phases = [ "nobuildphase" ];

    nobuildPhase = ''
      echo
      echo "This derivation is not meant to be built, aborting";
      echo
      exit 1
    '';

    shellHook = ''
      dir=`mktemp -d`
      trap 'rm -rf "$dir"' EXIT

      export cmakeDir=${builtins.toPath ./.}

      echo "Configuring into temporary directory (which will be removed on exit)..."
      echo "  Source directory = $cmakeDir"
      echo "  Temp directory   = $dir"
      echo "  target           = ${target}"
      cd $dir
      cmakeConfigurePhase

      make ${target} -j1
    '';
  });
in {
  check = format-shell "check-format";
  update = format-shell "update-format";
}
