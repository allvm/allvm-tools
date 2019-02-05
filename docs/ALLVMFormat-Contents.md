# Format Contents

The basic format of the `.allexe` is a zip container that
contains various files.

The types of files are described below.

## Bitcode

The main relevant file in the container is the `main.bc`
file, that contains the main bitcode for the executable.
This file is always required.

Additional bitcode files may be included directly, or may be
specified indirectly via _uniquely resolved_ identifiers;
in particular, no external specifications such as search paths
or other environmental variables should be used to resolve
these identifiers.
These additional bitcode files are expected
to be linked into `main.bc` in an unspecified manner.

Non-bitcode software components, such as scripts or non-LLVM-compiled
languages are _not_ included in ALLVM-format files.
In particular, all code components in these file must use LLVM bitcode.

## Non-code Components

### Software Data

Files other than code may reside in the package,
and all read-only data required by the application
for execution and available before shipping
should be included.
Sources of other read-only data should be specified
via configuration files.

Additionally, configuration files are encouraged
to be shipped as well. Users may modify the configuration
but do so sparingly, generating a new package instance
by doing so.

### ALLVM data

Other files in the container may be used for various
ALLVM-specific ends:

- analysis summaries,
- executable code caches
- optimized file dependencies

These are optional (although certain components may
require them), and it might make more sense to ship these
results outside of the package, for ease of distribution.
More work on use cases is needed.

For now, both models (i.e., files included or not) are supported.

## Metadata

Mandatory file with fixed name `metadata.inf`.

Contains following mandatory and optional fields:

- TODO

## Non-Code Data

** UNCLEAR HOW THIS IS DIFFERENT FROM ALL THE OTHER NON-CODE DATA, ABOVE.**

Assets, resources, \$PREFIX/share/\*
