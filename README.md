# ALLVM

## Contact

**Mailing List**: https://lists.cs.illinois.edu/lists/info/allvm-dev

**Chat** IRC: [#allvm on OFTC](irc://irc.oftc.net/allvm) (use an IRC client, or [using Riot](https://riot.im/app/#/room/#_oftc_#allvm:matrix.org))

**Website**: http://allvm.org

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


## How to build (without using allvm-nixpkgs)

### Requirements

The main requirement is LLVM.
To ensure your built LLVM will work and contains the required functionality:

* Use a supported version.  The latest version tested and known to work is tracked here: [llvm-version-info.log](https://gitlab-beta.engr.illinois.edu/llvm/allvm-nixpkgs/blob/master/llvm-version-info.log).
* Be sure to apply this [patch](https://gitlab-beta.engr.illinois.edu/llvm/allvm-nixpkgs/raw/master/pkgs/development/compilers/llvm/master/patches/llvm-R_X86_64_NONE.patch).
* Enable the `LLVM_INSTALL_UTILS` CMake option to ensure required tools like `FileCheck` are also installed.

### Building ALLVM Tools

After building LLVM as described above, you can build ALLVM as follows,
replacing `YOUR_LLVM_PREFIX` with the directory containing your installed LLVM:

```console
$ mkdir build && cd build
$ cmake -DLLVM_DIR=YOUR_LLVM_PREFIX/lib/cmake/llvm ..
$ make check -j$(nproc)
```

If you installed llvm to `/usr`, you can leave out the `-D LLVM_DIR=...` option
and CMake will find llvm automatically.


## Troubleshooting

See the [issues page](https://gitlab-beta.engr.illinois.edu/llvm/allvm/issues) for known problems or to report a new one.

The following issue is believed to be fixed regardless of whether allvm is built with GCC or Clang.
However it frequently crops up again.
If you get these errors in the latest version of ALLVM please add a comment to the issue.

* [Errors in RuntimeDyldELF.cpp when running alley](#1)


## Coding Style

Canonical coding style reference is the [LLVM Coding Standards](http://llvm.org/docs/CodingStandards.html) document,
and code should be formatted with an appropriate `clang-format`.  This process has been automated, as described below.

### Automatic Formatting and Checking

Build the `check-format` target to check that all files pass format style applied by `clang-format`.

If this fails, you may consider updating the source with the `update-format` target.

All contributed code should pass these checks.  Currently using clang-format corresponding
to LLVM version used to build the tools, we may pin a particular version in the future.
