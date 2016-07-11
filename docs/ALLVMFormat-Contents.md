# Format Contents

The basic format of the `.allexe` is a zip container that
contains various files.

The types of files are described below.

## Bitcode

The main relevant file in the container is the `main.bc`
file, that contains the main bitcode for the executable.
This file is always required.

Additional bitcode files may be provided, which are expected
to be linked into `main.bc` in an unspecified manner.

## Non-bitcode

### Software Data

Files other than code may reside in the package,
and all read-only data required by the application
for execution should be included.

Additionally, configuration files are encouraged
to be shipped as well.  Users may modify the configuration
but do so sparingly, generating a new package instance
by doing so.

### ALLVM data

Other files in the container may be used for various
ALLVM-specific ends:

* analysis summaries,
* executable code caches
* optimized file dependencies

These are optional (although certain components may
require them), and it might make more sense to ship these
results outside of the package, more work on use cases is needed.

For now, both models are supported.

## Metadata

Mandatory file with fixed name `metadata.inf`.

Contains following mandatory and optional fields:

* TODO

## Non-Code Data

Assets, resources, $PREFIX/share/*


