let
  jobs = rec {
    allvm-tools = import ./default.nix { nixpkgs = <nixpkgs>; };
  };
in jobs
