# ALLVM

[![Build Status](https://img.shields.io/travis/allvm/allvm-tools.svg?branch=master)](https://travis-ci.org/allvm/allvm-tools)

![ALLVM ALL THE THINGS!](https://img.shields.io/badge/ALLVM-ALL%20THE%20THINGS-brightgreen.svg)

## Contact

**Mailing List**: https://lists.cs.illinois.edu/lists/info/allvm-dev

**Chat**: #allvm on [OFTC](https://www.oftc.net/)
  (using any IRC client or chat in your browser using
  [Webchat](https://webchat.oftc.net/?nick=&channels=%23allvm&uio=d4) or
  [Riot](https://riot.im/app/#/room/#_oftc_#allvm:matrix.org))

Everyone is welcome to join!

**Website**: http://allvm.org

## Introduction to ALLVM

**FEAST'17 Keynote**: [Slides (pdf)](https://tc.gtisc.gatech.edu/feast17/papers/allvm.pdf) [Program (with talk abstract)](https://tc.gtisc.gatech.edu/feast17/program.html)


## Quickstart

### Nix

Install [Nix](https://nixos.org/nix).
Nix 2.0 or later is recommended.

### Installation

To build and install into your profile, run:

```console
$ nix-env -f . -i
```

### Building
From the root of the source directory, run:

```console
$ nix-build
```

The built result will be available in `./result`.

### Development

To enter a development shell with all dependencies available, run:

```console
$ nix-shell
```

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
[nix-shell:~/allvm-tools/build]$ cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/../install
[nix-shell:~/allvm-tools/build]$ make check -j
```


## How to build (without using Nix)

### Requirements

The main requirement is LLVM.
To ensure your built LLVM will work and contains the required functionality:

* Use a supported version.  The currently supported version is LLVM 4.0.
* Enable the `LLVM_INSTALL_UTILS` CMake option to ensure required tools like `FileCheck` are also installed.

### Building ALLVM Tools

After building LLVM as described above, you can build ALLVM as follows,
replacing `YOUR_LLVM_PREFIX` with the directory containing your installed LLVM:

```console
$ mkdir build && cd build
$ cmake -DLLVM_DIR=YOUR_LLVM_PREFIX/lib/cmake/llvm ..
$ make check -j$(nproc)
```
You only need to set `-D LLVM_DIR=...` when cmake has trouble finding your LLVM installation.

## Troubleshooting

See the [issues page](https://github.com/allvm/allvm-tools/issues) for known problems or to report a new one.


## Coding Style

Canonical coding style reference is the [LLVM Coding Standards](http://llvm.org/docs/CodingStandards.html) document,
and code should be formatted with an appropriate `clang-format`.  This process has been automated, as described below.

### Automatic Formatting and Checking

Build the `check-format` target to check that all files pass format style applied by `clang-format`.

If this fails, you may consider updating the source with the `update-format` target.

All contributed code should pass these checks.  Currently using clang-format corresponding
to LLVM version used to build the tools, we may pin a particular version in the future.
