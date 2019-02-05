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

The platform interface helps define
what interface(s) the software bitcode is able to use
and defines their behavior. This functionality
defines what it means for any software to run
on ALLVM and allows us to statically reason about
program behavior in terms of these operations.
The platform interface is likely to have multiple levels
of abstraction, e.g., at least one for userspace and
one for kernel code.

## Interfaces

### `libc` For All Userspace Software

Renowned for its small size and standards-compliant behavior,
software should be built against `musl` libc.

`musl` libc combines functionality of `libpthread`,
`libm`, `librt` and others, providing a solid
and familiar foundation for commodity software.

All the implications of this need to be further clarified,
e.g., what about software that requires GNU-specific
features in glibc, or software that uses other POSIX-compliant
libc features not supported by `mustl` libc.

### Multithreaded Software

All multithreaded software, in particular, must be able to run
on the Posix threads API, provided by libpthread.
Higher-level task, thread and synchronization libraries may be
implemented on libpthread, but the core ALLVM tools may not
recognize their interfaces and semantics, and so may be unable
to analyze or transform them effectively.
Customized tools, however, should be able to do so, by building
on core ALLVM compiler components.

### Other libraries

TBD:

- C++: libc++/libc++abi
- compiler runtime: compiler-rt instead of libgcc

### System ABI

TODO

## Technical Details

- Single entry point: `main.bc` defines function `main`.
- Static constructors/destructors supported as usual.
- Limited filesystem view
- User may provide alternative definitions of system code,
  for example by providing their own `malloc` implementation.
  Note that we do not provide the equivalent of `RTLD_NEXT`
  to find the original system function.
- No syscalls directly (no assembly), use musl interface
