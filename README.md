# ALLVM

## Building with allvm-nixpkgs

### Building
If you just want a built version of this repository, run the following:
```console
$ nix-build '<allvm>' -A allvm-tools
```

or equivalently (if you don't have `NIX_PATH` configured for allvm):
```console
$ nix-build /path/to/allvm-nixpkgs/allvm -A allvm-tools
```

And if you want to make it easy to use them while doing other tasks:
```console
$ nix-env -f '<allvm>' -iA allvm-tools
```

### Development

If you're interested in working on the tools themselves,
you'll want do be able to build yourself and to use
sources as you change them.

Nix has a very useful tool called `nix-shell` that can
be used to enter a shell that has all the needed dependencies
ready to go:
```console
$ nix-shell '<allvm>' -A allvm-tools
[nix-shell:~/allvm-tools]$ # easy as that
```
This shell is 'impure' and retains elements of your normal shell,
which usually is a good mix (so your editor/etc are still available).

If this causes problems, you can request a more pure shell
with the `--pure` argument which produces a cleaner environment
similar to that Nix uses when building allvm-tools itself.

Once in the shell, you can proceed to configure and build
as you would normally:

```console
[nix-shell:~/allvm-tools]$ mkdir build && cd build
[nix-shell:~/allvm-tools/build]$ cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=$PWD/../install
[nix-shell:~/allvm-tools/build]$ make -j
```


## How to build (without using allvm-nixpkgs)

This is tested with llvm-trunk (llvm 4.0?). You will need a copy of llvm built
with this [patch](https://gitlab-beta.engr.illinois.edu/llvm/allvm-nixpkgs/raw/master/pkgs/development/compilers/llvm/master/patches/llvm-R_X86_64_NONE.patch).
Install llvm somewhere. Then you can build ALLVM as follows, replacing
`YOUR_LLVM_PREFIX` with the directory you installed llvm to:

```console
$ mkdir build && cd build
$ cmake -D LLVM_DIR=YOUR_LLVM_PREFIX/lib/cmake/llvm ..
$ make -j$(nproc)
```

If you installed llvm to `/usr`, you can leave out the `-D LLVM_DIR=...` option
and CMake will find llvm automatically.

## Coding Style

Canonical coding style reference is the [LLVM Coding Standards](http://llvm.org/docs/CodingStandards.html) document,
and code should be formatted with an appropriate `clang-format`.  This process has been automated, as described below.

### Automatic Formatting and Checking

Build the `check-format` target to check that all files pass format style applied by `clang-format`.

If this fails, you may consider updating the source with the `update-format` target.

All contributed code should pass these checks.  Currently using clang-format corresponding
to LLVM version used to build the tools, we may pin a particular version in the future.

## Troubleshooting

See the [issues page](https://gitlab-beta.engr.illinois.edu/llvm/allvm/issues) for known problems and to report a new one.

The following issues are believed to be fixed but are listed here in case they crop up again.
Please let us know if you encounter them in the latest version:

* [assertion failure "isInt<32>(RealOffset)" when using alley to JIT an allexe](#1)
