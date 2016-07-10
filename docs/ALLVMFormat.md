% ALLVM Package Format Design

# Overview

The basic format of the `.allexe` is a zip container that
contains various files.

## Purpose

The ALLVM Package Format is used by the ALLVM tools and
built by the ALLVM build system automation.

The format's primary design goal is to represent common
'software' components in a way that is:

* Easy to generate from reasonably organized software
* Sufficient for execution of typical user workloads
* Amenable to compiler analysis and transformations

The next section expands on the design goals further.

## [Design Goals](ALLVMFormat-Design.md)

## [Non-Goals](ALLVMFormat-NonGoals.md)

(Follow link to section contents)

# Zip Contents

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

# [Platform Interface](ALLVMFormat-Platform.md)

# [TODO](ALLVMFormat-TODO.md)
