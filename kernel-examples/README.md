# Kernel-mode examples

Both examples include only `lendiza_core.hpp` — the kernel-safe umbrella. They
pull in no STL, no exceptions, no RTTI, no virtual dispatch and no CRT object
file, which is what makes the decoder usable at `DISPATCH_LEVEL` / in interrupt
context.

## Windows (`win/`)

| file | role |
|---|---|
| `lendiza_decode.cpp` | C++ shim: calls the decoder, maps error codes to `NTSTATUS` |
| `lendizak.c` | driver entry: decodes a static buffer and its own code page; shows the probe-and-copy path for user memory |
| `lendiza_ksample.h` | `extern "C"` prototypes shared by both |

Compile check (this is what CI runs — it needs VS2022 + WDK, no signing):

```bat
scripts\check_kernel_compile.cmd
```

It compiles all three TUs with

```
cl /kernel /std:c++17 /GR- /EHs-c- /W4 /D_AMD64_=1
   /I<WDK>\Include\<ver>\km\crt /I<WDK>\Include\<ver>\km /I<WDK>\Include\<ver>\shared
   /I<lendiza>\include
```

and then checks each object with `dumpbin /symbols` for `__CxxFrameHandler`,
`_purecall`, `_initterm`, `_CxxThrowException`, `__security_init_cookie` or any
RTTI symbol (`??_R`). None are present: the code references only ntoskrnl.

To get a loadable `.sys`, create a driver project from the WDK template
(`File > New > Project > Windows Driver > KMDF/Non-PnP`, or an existing
`.vcxproj` with `ConfigurationType=Driver`) and add `lendizak.c` plus
`lendiza_decode.cpp` to it — the include directory is all the library needs.
Loading needs test signing (`bcdedit /set testsigning on`) and a signed or
test-signed catalog, so it is deliberately not automated here.

Note `include/lendiza.hpp` is **not** kernel-safe (it uses `std::span`); include
`lendiza_core.hpp` as these examples do.

## Linux (`linux/`)

| file | role |
|---|---|
| `lendiza_kshim.cpp` | C++ translation unit exposing `lendiza_kshim_decode/walk` |
| `lendiza_kmod.c` | module init: decodes a static buffer and its own text; `copy_from_user` before decoding user memory |
| `Makefile` | kbuild file with the custom rule that compiles the `.cpp` |

Compile check:

```sh
sh scripts/check_kernel_compile.sh
```

Step 1 always runs: it compiles the core with
`g++ -std=gnu++17 -ffreestanding -nostdinc++ -fno-exceptions -fno-rtti
-fno-builtin` and then `nm -u` to prove no `operator new`, `__cxa_*`,
`_Unwind_*`, RTTI or personality-function symbol is referenced. Step 2 runs the
guard-page probe natively. Step 3 builds the real module when kernel headers are
installed.

Building the module needs headers for your running kernel:

```sh
# Debian/Ubuntu
sudo apt install linux-headers-$(uname -r)
make -C /lib/modules/$(uname -r)/build M=$PWD modules
sudo insmod lendiza_demo.ko && dmesg | tail
sudo insmod lendiza_demo.ko decode_addr=0x7ffd1234 long_mode=0
```

(`lendiza_demo.ko`, not `lendiza_kmod.ko`: kbuild generates a composite object
named after the module target, so the target must not share the `.c` file's stem.)

`-nostdinc++` is the load-bearing flag: out-of-tree C++ has no libstdc++ in the
kernel, and the core does not ask for one. The `.o` is linked by kbuild like any
other object; nothing else in the module is C++.

Two things the real kbuild run settled:

- **The C++ TU includes no kernel headers.** Kernel headers are not C++-clean
  before v6.15: `asm-generic/rwonce.h` needs the kernel's own C++ helpers, and
  `compiler_types.h` rewrites `auto` to `__auto_type` and `inline` to
  `inline __gnu_inline`. The decode core needs no kernel API, so `lendiza_kshim.cpp`
  stays kernel-header-free and the C half maps raw results onto `errno`
  (`-ENOSPC` for a truncated window, `-EILSEQ` for an undecodable one).
- **The module target must not share the `.c` file's stem**, because kbuild
  generates a composite `<target>.o`; `lendiza_kmod.c` + `lendiza_kmod-y` is a
  circular dependency. Hence `lendiza_demo.ko`.

Verified on this machine against `linux-headers-6.1.0-53-amd64`: `CC [M]`,
`CXX`, `LD [M]`, `MODPOST` all succeed and produce a 220 KB `lendiza_demo.ko`;
`nm -u` on the C++ object shows no `operator new`, `__cxa_*`, `_Unwind_*` or RTTI
symbol. That build tree does not match the running WSL2 kernel, so this proves
compile and link, not loading.
