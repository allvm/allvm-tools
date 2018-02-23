let
  spec = builtins.fromJSON (builtins.readFile ./nixpkgs-src.json);
  fetchTarball = if builtins.lessThan builtins.nixVersion "1.12" then
    { url, sha256 }: builtins.fetchTarball url
    else builtins.fetchTarball;
in
  fetchTarball {
    url = "https://github.com/${spec.owner}/${spec.repo}/archive/${spec.rev}.tar.gz";
    sha256 = spec.sha256;
  }
