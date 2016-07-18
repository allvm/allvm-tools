# Platform Interface

The `allexe` format is very much an executable file format,
which means its definition is closely coupled with the
specification of expected runtime behavior.

This section describes the ALLVM platform and the interface
it provides to the software running on top of it.

Mostly this should be pinning down what portions
of existing interfaces are supported and specifying
that it's safe to assume the functionality of those
methods are what those libraries define them to be.

Additionally, this allows ALLVM to use bitcode
implementations of these interfaces to potentially
further optimize and analyze software.

## Why is this needed

The platform interface helps define that by clarifying
what interface(s) the software bitcode is able to use
and defines their behavior.  This functionality
defines what it means for their software to run
on ALLVM and allows us to statically reason about
program behavior in terms of these operations.

## Interfaces

### Underneath it all: `libc`

Renowned for its small size and standards-compliant behavior,
software should be built against `musl` libc.

What exactly this means needs to be further clarified.

`musl` libc combines functionality of `libpthread`,
`libm`, `librt`, and others providing a solid
and familiar foundation for commodity software.

### Other libraries

WIP:
* C++: libc++/libc++abi
* compiler runtime: compiler-rt instead of libgcc

### System ABI

TODO

## Technical Details

* Single entry point: `main.bc` defines function `main`.
* Static constructors/destructors supported as usual.
* Limited filesystem view
* User may provide alternative definitions of system code,
  for example by providing their own `malloc` implementation.
  Note that we do not provide the equivalent of `RTLD_NEXT`
  to find the original system function.
* No syscalls directly (no assembly), use musl interface
