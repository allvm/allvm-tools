with import <allvm> {};

# Temporary helper for building using same environment
# used in the <allvm> channel, but using this source
# instead of what is normally used.

# This seems to "fix" most tests that otherwise crash,
# investigation neeed!

# In the meantime it's useful to build this way
# (even if it only gets lucky avoiding the problem)
# so adding this here.

# Use:
# $ nix build -f ./nix/build-as-allvm-channel-override.nix
# (Note this requires configuration and access to the
# allvm channel which is not yet public)

allvm-tools.overrideAttrs(o: { src = fetchGit ../.; })
