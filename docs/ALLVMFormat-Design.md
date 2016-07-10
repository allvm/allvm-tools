# Design Goals

These overlap a bit as they develop, bear with us :).

## Easy to Inspect with Commodity Tools

ZIP, bitcode, human-readable metadata, etc.

We provide tools to operate on the format of course,
but the format is designed to be easily created, inspected,
and consumed for the cases our tools don't cover.

This is important for our own sanity, but also to ease
adoption and construction of tools interacting with the
format.

## Completeness

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

## All Code is in LLVM-IR

In order to facilitate analysis and transformations,
all code must be provided in bitcode format.

For now this means code using assembly for any reason
is not allowed, including the use of inline assembly
as well as standalone assembly files.

This is likely too strict a requirement for all uses,
and mechanisms for handling those will be provided
in later versions of the format.

## Dynamic behavior is Statically Predictable

(As much as possible!)
Software requiring this is encouraged to evalute if it
really needs the flexibility, and if so may elect to
indicate this need with a flag.

## Isolated-By-Default

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

## Reproduction and Caching

## Simple by Default

Capture the majority of use cases with what's simplest,
introduce complexities only as-needed (if at all).

## Suitable for Analysis and Transforms

TODO: What does this mean? For now trying to stub out.

#### Associating and Storing Analysis Results for Software

#### Offline Compiler Techniques

#### Online Compiler Techniques
