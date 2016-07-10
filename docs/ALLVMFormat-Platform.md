# Platform Interface

## Why is this needed

Just in case it's not clear, we unfortunately cannot
reasonably avoid specifying this IMO.

An explanation of why belongs here :).

## Summary

* C: musl
  * musl interface provided by runtime
  * do not link statically
* C++: libc++/libc++abi
* compiler runtime: compiler-rt instead of libgcc
* No syscalls directly, use musl interface
* "POSIX"?
  * libpthread, etc.?
* TODO: Leverage existing ABI specifications if can find a
  suitable one to build on?

## Technical Details

* Single entry point: `main.bc` defines function `main`.
* Static constructors/destructors supported as usual.
* Limited filesystem view

