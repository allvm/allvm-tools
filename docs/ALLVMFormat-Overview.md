# Overview

This document is an informal specification of the file format
used for software running in an ALLVM system.
We use the following extensions for such files:

* .allexe -- self-contained executable files, e.g., for an application

More may be added in the future.

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


