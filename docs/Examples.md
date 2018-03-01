# Examples

The allvm-tools are used in conjunction with allvm-nixpkgs to produce
software built entirely as LLVM IR, in the form of "allexes".

We are in the process of making allvm-nixpkgs public,
but in the meantime we provide the following examples
to demonstrate the functionality and possibilities of ALLVM.

## Preliminaries

We'll be using the ALLVM binary cache located at https://cache.allvm.org .

You can configure Nix to use and trust this cache by following the instructions
available here (TODO).

In these examples, however, we'll be taking advantage of new Nix features to
obtain and run prebuilt examples from a local store without requiring
your system to trust our builders, although signatures will be checked.

### Notes

**Local store**:
The new nix commands (`nix run`, `nix build`, etc.) allow specifying a custom
store URI with the `--store` parameter. We will use this so that
nothing system-level is modified and no system-level trust is required.

**bash and coreutils**:

When using `nix run` we often want a few utilities beyond those in the example,
particularly a shell.  The aliases below provide `bash` and `coreutils`
for convenience and as the first example.

**Closure sizes**

In our prototype, path closures are often very large due to references
retained to previous build stages and compilers.

This is harmless (other than wasting space) but as a result checking
the sizes of the multiplexed applications must be done carefully
as a simple closure size check will be misleading.

This must be done anyway when comparing code size since paths
(and their closures) contain non-code files as well,
but is especially important due to our bloated closures :).

We hope to fix this soon.

**Fixup**:

TODO: explain the "fixup" phase in producing multiplexed examples--
what it is, why is it needed, alternatives and future work.

**AOT Wrappers**:
For execution convenience and to facilitate measuring the size of the
resulting static multiplexed binaries, wrappers are emitted
that set `ALLVM_CACHE_DIR` to point to pre-built caches
generated using the `allready` tool.

For example:
```console
$ cat /nix/store/0rs61708farxdknpb6v5cfz2jkb1imm4-memcacheds-allready/bin/memcached-1-4-0
#! /nix/store/fmwr829knwxl5qcf4b4m27cixvcr15in-bash-4.3-p46/bin/bash -e
export ALLVM_CACHE_DIR="/nix/store/qzgraxd2mjyrddcccp70zv29x8n332yd-memcacheds-allready-cache"
exec /nix/store/27g15wxxg6531ja2cv7rfnad5v26dk52-memcacheds-mux-merged/bin/memcached-1-4-0 "${extraFlagsArray[@]}" "$@"
```

This is a bit of a hack and can cause unexpected behavior
(cache misses, failure to write to the cache)
in child processes should they invoke allexe's
not included in this prebuilt cache.

In the future `ALLVM_CACHE_DIR` may be a list of paths,
although there are some problems with this that will
need to be addressed-- or another solution found.

## Aliases

The following examples assume aliases such as:
```console
$ alias allvm-run-base='nix run --store $HOME/allvm-store --option binary-caches "https://cache.nixos.org https://cache.allvm.org" --option trusted-binary-caches "gravity.cs.illinois.edu-1:yymmNS/WMf0iTj2NnD0nrVV8cBOXM9ivAkEdO1Lro3U= cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="'
$ alias allvm-run='allvm-run-base /nix/store/k131gci7nr2f80daim20zbilc1x6s6fy-bash-interactive-4.4-p12-wllvm-allexe/bin/bash /nix/store/pd1ikvjzh43cgb372bz86msqmlmd70pj-coreutils-8.29-wllvm-allexe'
```

The first simply saves us a bunch of typing when invoking `nix run`,
and the second includes two paths with allexes for bash and coreutils
as they are commonly useful.

You may consider adding to your shell's init script (example: `~/.bashrc`).

## ALLVM Cache

The default location of the cache used by `alley` is `$XDG_CACHE_HOME/allvm`
which usually means `~/.cache/allvm`.

## First Allexe: Bash and Coreutils

To run an allexe-based bash shell:
```console
$ allvm-run
```

This will execute the bash allexe using `alley`, taking a few seconds on first execution
... because it is being JIT-compiled fresh for you!
Per-module translations are cached-- try exiting the shell
and running it again and you'll notice it starts much faster.

You can inspect this allexe with the `all-info` tool (using prebuilt version here for convenience):

```console
$ allvm-run /nix/store/3niq9g1pba5hb452wh9rqcyxp480x8sr-allvm-tools-git-5f7ad6e \
  -c all-info /nix/store/k131gci7nr2f80daim20zbilc1x6s6fy-bash-interactive-4.4-p12-wllvm-allexe/bin/bash
Modules:
	main.bc (17240804)
	/nix/store/v3bvn8ga5j0jx8scykb1bllkgvb3ckai-ncurses-6.0-20180106-wllvm/lib/libncursesw.so.6.0.bc (DDA54A1E)
```

Try running various coreutils programs such as `du`, `ls`, `tail`, and `whoami`. Each are compiled on-demand into the cache, notice they execute faster after first use.

## xterm

The default xterm allexe can be obtained and inspected with:
```
$ allvm-run \
	/nix/store/kl3jiag59rdfrj4p5p0zpcg7cjs73vqw-xterm-331-wllvm-allexe \
	/nix/store/3niq9g1pba5hb452wh9rqcyxp480x8sr-allvm-tools-git-5f7ad6e \
$ all-info /nix/store/kl3jiag59rdfrj4p5p0zpcg7cjs73vqw-xterm-331-wllvm-allexe/.xterm-wrapped
Modules:
	main.bc (3A001D08)
	/nix/store/nnqv6sq1av5cl81rdjs97q4sr7b4hays-libX11-1.6.5-wllvm/lib/libX11.so.6.3.0.bc (289832E1)
	/nix/store/5ph8nza512vfkx36xhswqpagklvzhfah-util-linux-2.30.2-wllvm/lib/libuuid.so.1.3.0.bc (92596651)
	/nix/store/qrzn9vngmkpc630dmmh2j3fgs35jvmil-libXft-2.3.2-wllvm/lib/libXft.so.2.3.2.bc (C01C9B66)
	/nix/store/977dxvp8hbl6b8wdijbz4b0cdfizk2nn-libXaw-1.0.13-wllvm/lib/libXaw7.so.7.0.0.bc (4D533DA1)
	/nix/store/v3bvn8ga5j0jx8scykb1bllkgvb3ckai-ncurses-6.0-20180106-wllvm/lib/libncursesw.so.6.0.bc (DDA54A1E)
	/nix/store/n4hkif6zm3wxvhqg0a8s2dm2l4wf1n1l-libXrender-0.9.10-wllvm/lib/libXrender.so.1.3.0.bc (DA879BDE)
	/nix/store/9fliwyn9j9bk7i4a1n7x8i4pccaari3r-bzip2-1.0.6.0.1-wllvm/lib/libbz2.so.1.0.6.bc (8EB538A3)
	/nix/store/c92kwp94wn4nigyb97mwx1npdgv4yqiz-zlib-1.2.11-wllvm/lib/libz.so.1.2.11.bc (925FD9DA)
	/nix/store/l7x9xvrkb0gdz6iqj354qlsyrqb1xagm-libXdmcp-1.1.2-wllvm/lib/libXdmcp.so.6.0.0.bc (45944FA0)
	/nix/store/brql7g2743hm5r51iz36zbvs4h0x5pg4-freetype-2.6.5-wllvm/lib/libfreetype.so.6.12.5.bc (6C958D8F)
	/nix/store/w92yhgr67sx5wywdjcyc7bmqd7abw0ww-fontconfig-2.12.1-wllvm-lib/lib/libfontconfig.so.1.9.2.bc (E7DDDBE4)
	/nix/store/q3d2ljgpj1lhsy34ai06gq3dn28r0d7x-libSM-1.2.2-wllvm/lib/libSM.so.6.0.1.bc (E82732CF)
	/nix/store/ipm0g6llgkj9jydl79dpc3wzmwhbxypn-libxcb-1.12-wllvm/lib/libxcb.so.1.1.0.bc (2753C67B)
	/nix/store/c8zc20pafk911n32mpxcya4id2k3pny1-libXext-1.3.3-wllvm/lib/libXext.so.6.4.0.bc (69DFF284)
	/nix/store/700pa1n8ipms8q460zqagmhsczhxlrq2-libpng-apng-1.6.26-wllvm/lib/libpng16.so.16.26.0.bc (20A59698)
	/nix/store/j4bpygdh41cv7w6mcdj3qq4mj468dy80-libXpm-3.5.12-wllvm/lib/libXpm.so.4.11.0.bc (E96F04A8)
	/nix/store/vy5zn0w03dlhp0qw6pw4ncrdc9drbcny-libXmu-1.1.2-wllvm/lib/libXmu.so.6.2.0.bc (5CF7C7D8)
	/nix/store/z3pidigd9khwpvimwl35j86xny7dp4dv-expat-2.2.4-wllvm/lib/libexpat.so.1.6.6.bc (7D84ED4F)
	/nix/store/bvpxvzv7ij0yjgnsxxrfdx4y2lhj8c98-libICE-1.0.9-wllvm/lib/libICE.so.6.3.0.bc (C42AD66C)
	/nix/store/1cxi40b6hn2a0l9dypr72a0h1fl9czgl-libXt-1.1.5-wllvm/lib/libXt.so.6.0.0.bc (A1255BEA)
	/nix/store/qw2h35s1r5z5513ckzxwwm3idm0p8ji5-libXau-1.0.8-wllvm/lib/libXau.so.6.0.0.bc (DC0ADC24)
```

(Note that the binary is called `.xterm-wrapped`)

This will not currently run with alley due problems resolving symbols.
You can create a merged version with the `alltogether` tool--here we'll save ourselves
the trouble and grab it from the cache:

```console
$ allvm-run /nix/store/0dn6pknqvkny98sbja2fxdwkgqqmlbxk-xterm-331-wllvm-allexe -c .xterm-wrapped -e bash
```

This will take a while to compile, but eventually an xterm should appear!
Running it in the future will be quick, try it and see!

## Binutils

/nix/store/n3vn9rc13qra4jgrvrr6srzldd2vmv5j-binutils-2.28.1-native-wllvm-allexe

## Termite (terminal emulator)

Normal: /nix/store/xx6mjc5c878lw24wqx820m4j5wicpg8q-termite-13-wllvm-allexe

Merged: /nix/store/2ajs7b4ni9kzrxyv3jrrnqz4gqrqnbjn-termite-13-wllvm-allexe


## Additional Allexe Paths

We have thousands of packages built into allexes,
a few of which we list below.

| Name | Path | Notes |
|----|----|----|
| 2048 | /nix/store/mzjfldvv0v7knhgb6yyhnpdfzkb0mk1v-2048-in-terminal-2017-11-29-wllvm-allexe | |
| apache httpd | /nix/store/1r86ny0lcfv8kzzrcpylbj90ikwlrvnp-apache-httpd-2.4.29-wllvm-allexe/bin/httpd | |
| git | /nix/store/f3wqx0hz0qd1kyjna8xrb6kb3ab2vgqj-git-minimal-2.16.2-wllvm-allexe | |
| haproxy | /nix/store/g6vix9mvfrpnl1prz98r78qndjmal9z9-haproxy-1.7.9-wllvm-allexe | |
| openssh | /nix/store/4x1zkgnc3sh8fqn7vhiv42qvf2a8zn3r-openssh-7.6p1-wllvm-allexe | |
| rdesktop | /nix/store/6n3w8ivj620nm8f9sdwyqz8s91207bk6-rdesktop-1.8.3-wllvm-allexe | |
| svn | /nix/store/2i6rv5w50gjzyadihii111zcs484rm6z-subversion-client-1.9.6-wllvm-allexe/ | |
| zsh | /nix/store/r13rw8rgqah5467mvbwj2x9wgksr7jy2-zsh-5.4.2-wllvm-allexe | |


## Multiplexed Collections

### Common Path

Mux'd: /nix/store/9spbpi5y8ily82qq2dmbq3if7r0q0nkh-common-path-mux
AOT: /nix/store/kszg4sc2q3qfwmzfblp7q30vclh2s1i2-common-path-allready

### Editors

AOT: /nix/store/ajvs697s6i59am6fm4lbsfpc7hqajz7a-editors-allready

### 40 Memcacheds in 1


List all of the included binaries with:
```console
$ allvm-run /nix/store/0rs61708farxdknpb6v5cfz2jkb1imm4-memcacheds-allready \
  -c ls /nix/store/0rs61708farxdknpb6v5cfz2jkb1imm4-memcacheds-allready/bin
```

All of which execute from the same binary.  To enter a shell with all on your PATH, run:
```console
$ allvm-run /nix/store/0rs61708farxdknpb6v5cfz2jkb1imm4-memcacheds-allready
bash-4.4$ memcached-1-5-1 --version
memcached 1.5.1
```

Note that early versions do not support the `--version` flag, and the flag used for help
has changed over time as well.
