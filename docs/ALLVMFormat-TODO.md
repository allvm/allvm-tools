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

