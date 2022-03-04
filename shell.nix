{ ... } @ args:
let nargs = if (args?nixpkgs) then { nixpkgs = args.nixpkgs; } else {}; in
# Pick single attr, but disable Werror flags since they
# are likely annoying during development.
(import ./default.nix nargs).allvm-tools.override { useClangWerrorFlags = false; }
