# ALLVM Package Format Design

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

## Design Goals

These overlap a bit as they develop, bear with us :).

### Easy to Inspect with Commodity Tools

ZIP, bitcode, human-readable metadata, etc.

We provide tools to operate on the format of course,
but the format is designed to be easily created, inspected,
and consumed for the cases our tools don't cover.

This is important for our own sanity, but also to ease
adoption and construction of tools interacting with the
format.

### Completeness

Software in this format is captured in sufficient
completeness that it may be executed by the ALLVM Execution
Engine without additional code components.
It is encouraged to also include key resources as well,
including configuration files.

This has important implications regarding dynamically
linked libraries:
*all the libraries used by an application must be included*.
For ease of generation, updating, and analysis modularity,
these are encouraged to be shipped as separate bitcode files
with linking deferred until run-time.

### All Code is in LLVM-IR

In order to facilitate analysis and transformations,
all code must be provided in bitcode format.

For now this means code using assembly for any reason
is not allowed, including the use of inline assembly
as well as standalone assembly files.

This is likely too strict a requirement for all uses,
and mechanisms for handling those will be provided
in later versions of the format.

### Dynamic behavior is Statically Predictable

(As much as possible!)
Software requiring this is encouraged to evalute if it
really needs the flexibility, and if so may elect to
indicate this need with a flag.

### Isolated-By-Default

While security is not a primary goal of the file format,
and the execution engine is not required to enforce
isolation (yet), to enable and encourage aggressive
full-software analysis and transformations (such
as partial evaluation and similar) the behavior
of the software is limited unless indicated otherwise.

How this is most naturally described for software
is not yet determined, but for now includes:

* not being able to execute arbitrary code
* no self-modifying or JIT'd code
* limited file system access
  * provided assets are available read-only
* All other data declared explicitly as input to the software

### Reproduction and Caching

### Simple by Default

Capture the majority of use cases with what's simplest,
introduce complexities only as-needed (if at all).

### Compatibility?

(This could mean many things, what did you mean? :))

### Suitable for Analysis and Transforms

TODO: What does this mean? For now trying to stub out.

#### Associating and Storing Analysis Results for Software

#### Offline Compiler Techniques

#### Online Compiler Techniques


## Non-Goals

### Pretending Build Systems Are Great When They're Not

#### Cross-Compilation Features without Cross-Compilation Troubles

### Full Feature Compatibility with Everything

(ELF/ld.bfd/magic-perl-script-that-emits-asm-at-runtime)

Foremost goal is to enable compiler techniques for software,
only sacrificing that when the requirements are too strict
for use with common 'reasonable' software.
This is nebulous, but hopefully can be defined more
concretely in the future.

### Package Manager Features

For now anyway, this format is probably better called
something other than a 'package', as it doesn't attempt to
solve or be suitable for:

* Installation
* Expressing cross-package dependencies
  * Other formats my address this, however.
* Efficient distribution

That said, it is my hope that we can provide an ALLVM
Package Manager around this format.  For example, an
efficient distribution system could be built to provide
`allexe` files on-site, for example by using knowledge of
individual bitcode files that appear in many packages,
or doing incremental updates.  Format decisions
accomodating package manager goals are possible
down the road but for now are not important.

# Zip Contents

## Bitcode
The main relevant file in the container is the `main.bc`
file, that contains the main bitcode for the executable.
This file is always required.

Additional bitcode files may be provided, which are expected
to be linked into `main.bc` in an unspecified manner.

## Non-bitcode

Other files in the container contain analysis summaries,
executable code caches, optimized file dependencies, etc.
Most of these files are optional.

## Metadata

Mandatory file with fixed name `metadata.inf`.

Contains following mandatory and optional fields:

* TODO

## Non-Code Data

Assets, resources, $PREFIX/share/*

# Platform Interface

## Why is this needed

## Summary

* C: musl
  * musl interface provided by runtime
  * do not link statically
* C++: libc++/libc++abi
* compiler runtime: compiler-rt instead of libgcc
* No syscalls directly, use musl interface

# TODO

* Pick names for the executable
  * (Will: What does this mean?)
* What Metadata?
  * Codegen flags? Format?
* Shared libraries + static constructors:
  * okay to handle as-if LD_BIND_NOW specified?
  * clarify lack of support for `dlopen` and friends.
* Format for metadata?
  * inf or json? We did decide on INF right?
* /usr/share-like data
* Debug information as optional
* headers? Can ALLVMexe's be used as build-time dependencies?
* Format specifiation versioning?
  * compliance checking, backwards-compat, etc.
