** How to build (without using allvm-nixpkgs)
This is tested with llvm-trunk (llvm 4.0?). You will need a copy of llvm built with this [patch](https://gitlab-beta.engr.illinois.edu/llvm/allvm-nixpkgs/raw/master/pkgs/development/compilers/llvm/master/patches/llvm-R_X86_64_NONE.patch)

Once the `llvm-config` in your `$PATH` is pointed to the patched llvm, you can just configure and make.
