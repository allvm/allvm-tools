# Overview

This document is an informal specification of the file format
used for software running in an ALLVM system.
We use the following extensions for such files:
* .allexe -- self-contained executable files, e.g., for an application
* .alllib -- statically linked libraries in ALLVM format
* .allso  -- dynamically loadable libraries in ALLVM format
All these files use a common format, which is a zip container that
contains various files.

## Purpose

The ALLVM Package Format is used by the ALLVM tools and
built by the ALLVM build system automation.

The format's primary design goal is to represent common
'software' components in a way that is:

* Easy to generate from reasonably organized software
* Sufficient for execution of typical user workloads
* Amenable to compiler analysis and transformations across all software
  boundaries

The next section expands on the design goals further.


