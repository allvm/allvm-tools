{ ... } @ args:
# Pick single attr, but disable Werror flags since they
# are likely annoying during development.
(import ./default.nix args).allvm-tools-clang.override { useClangWerrorFlags = false; }
