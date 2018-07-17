#!/usr/bin/env bash

cachix push allvm --watch-store &

nix-build $@ -o result | cachix push allvm


# If triggered by cron, ensure build closure is pushed too
if [[ $TRAVIS_EVENT_TYPE == "cron" ]]; then

  function push_paths () {
    echo "realizing paths..."
    nix build $@ --no-link
    local drvs=$(nix-store -q --deriver $@)
    echo "derivers: "
    echo $drvs | sed -e 's/^/  /'
    echo "realizing...."
    echo $drvs | xargs nix-store -r 2>&1 | sed -e 's/^/  /'
    echo "computing closure (including outputs....)"
    local closure=$(echo $drvs | xargs nix-store -qR --include-outputs)
    echo "closure size: $(echo $closure | wc -l)"
    echo "pushing..."
    echo $closure | cachix push allvm 2>&1 | sed -e 's/^/  /'
    echo "done!"
  }

  echo "Pushing build (and runtime) closures..."
  push_paths ./result*

else
  echo "Pushing runtime closures..."
  cachix push allvm ./result*
fi
