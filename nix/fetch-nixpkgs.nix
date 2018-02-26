let
  spec = import ./nixpkgs-src.nix;
in
  fetchTarball {
    url = "https://github.com/${spec.owner}/${spec.repo}/archive/${spec.rev}.tar.gz";
    name = spec.rev;
    sha256 = spec.sha256;
  }
