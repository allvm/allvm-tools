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
[nix-shell:~/allvm-tools/build]$ ../configure --enable-optimized
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
