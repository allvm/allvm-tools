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

## Architecture Portability

While the nature of the ALLVM platform provides some measure
of portability, in no way is this in an effort to make
it possible to run the same `allexe` everywhere.

## Security

While `allexe` provides many features that are helpful for
security purposes, the core system itself does not attempt
to make any guarantees about enforcing these properties.

Rather, these are asssumptions made to ease analysis
for 'most' software.

That said, security enforcement may be a useful
optional feature to build into execution
engines operating on `allexe` files down the road.



