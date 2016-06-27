The basic format of the `.allexe` is a zip container that contains various files.

The main relevant file in the container is the `main.bc` file, that contains the
main bitcode for the executable. This file is always required.

Other files in the container contain analysis summaries, executable code caches,
optimized file dependencies, etc. Most of these files are optional.

TODO:
* Pick names for the executable
* Enforce filenames in a particular order? Affects performance, etc.
* Format for codegen flags
* Handling linking concerns--particularly shared libraries
* Format for metadata?
* /usr/share-like data
* Debug information as optional
