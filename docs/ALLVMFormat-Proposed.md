ALLVM Package Format Design
===========================

Updated: 7/6/16

Format Description
==================

* Zip file of bitcode files with main.bc first followed by zero or more bitcode libraries.

Non-bc Files
============

* Use tar to preserve attributes easily
* Available to program as read-only.


Execution Specification
=======================

Execution may not do things this way, but code in ALLVM Format expects 'equivalent' treatment.

* Bitcode files linked (llvm-link --only-needed <main.bc> <lib*.bc>)
* Code corresponding to bitcode loaded into process.
* Static constructors executed (?)
* Execution starts from `main`
