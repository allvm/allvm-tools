# Non-Goals

## Pretending Build Systems Are Great When They're Not

### Cross-Compilation Features without Cross-Compilation Troubles

## Full Feature Compatibility with Everything

(ELF/ld.bfd/magic-perl-script-that-emits-asm-at-runtime)

Foremost goal is to enable compiler techniques for software,
only sacrificing that when the requirements are too strict
for use with common 'reasonable' software.
This is nebulous, but hopefully can be defined more
concretely in the future.

## Package Manager Features

For now anyway, this format is probably better called
something other than a 'package', as it doesn't attempt to
solve or be suitable for:

* Installation
* Expressing cross-package dependencies
  * Other formats may address this, however.
* Efficient distribution

That said, we hope that we can in future provide an ALLVM
Package Manager around this format.  For example, an
efficient distribution system could be built to provide
`allexe` files efficiently on-site, perhaps by using knowledge of
individual bitcode files that appear in many packages,
or by doing incremental updates.  
Moreover, such a distribution system would take advantage of 
ALLVM tools to carry out install-time optimizations such as
hardware-specific optimizations, software specialization, and autotuning.
Format decisions
accomodating package manager goals are possible
down the road but for now are not important.

## Architecture Portability

While the nature of the ALLVM platform provides some measure
of portability, in no way is this in an effort to make
it possible to run the same `allexe` everywhere.
Such portability requires extensive and carefuly designed
language-level support, which native languages such as C/C++
fundamentally lack.

## Security

While `allexe` provides many features that are helpful for
security purposes, the core system itself does not attempt
to make any guarantees about enforcing these properties.
Rather, the asssumptions stated earlier are made primarily
to ease analysis and transformations for 'most' software.

That said, the ALLVM format and tools should enable system designers to
build secure systems and to enforce rich security policies on top of a core
ALLVM system, as noted earlier.  In fact, we consider this to be an
important research direction building on ALLVM.



