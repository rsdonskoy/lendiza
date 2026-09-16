#!/bin/sh
# check_kernel_compile.sh - proves the decode core needs no C++ runtime, and (when
# kernel headers are installed) that it actually builds as a Linux module.
#
#   sh scripts/check_kernel_compile.sh            # from the repository root
#   CXX=g++-12 sh scripts/check_kernel_compile.sh
#
# Step 1 always runs and is the portable claim: the core compiles with
# -nostdinc++ -fno-exceptions -fno-rtti -ffreestanding, i.e. with no libstdc++
# headers or runtime available at all.  Step 2 needs matching kernel headers; it
# is skipped (clearly reported) when they are absent.
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
CXX=${CXX:-g++}
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

echo "== 1. hosted compile with no C++ runtime (no libstdc++, no exceptions, no RTTI) =="
"$CXX" -std=gnu++17 -Os -Wall -Wextra -Wno-unused-parameter \
    -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables \
    -nostdinc++ \
    -I"$root/include" \
    -c "$root/tests/kernel_compile_probe.cpp" -o "$out/probe.o"

# Nothing may pull in the C++ runtime: check for undefined symbols a kernel could
# not satisfy (operator new, exception/RTTI support, pure virtual, unwind tables).
if command -v nm >/dev/null 2>&1; then
    if nm -u "$out/probe.o" | grep -E '_Znw|_Znam|__cxa_|_Unwind_|__gxx_personality|_ZTIN|_ZTSN|_purecall|__cxa_pure_virtual'; then
        echo "[FAIL] the object references the C++ runtime"
        exit 1
    fi
    echo "[ OK ] object references no C++ runtime symbol"
else
    echo "[SKIP] nm unavailable, cannot inspect undefined symbols"
fi

echo
echo "== 2. guard-page over-read probe (native, needs no kernel headers) =="
if [ "$(uname -s)" = "Linux" ]; then
    "$CXX" -std=gnu++17 -O1 -Wall -I"$root/include" -I"$root/tests" \
        "$root/tests/guard_page.cpp" "$root/src/lendiza.cpp" -o "$out/guard_page"
    "$out/guard_page"
else
    echo "[SKIP] guard_page runs on Linux and Windows; on Windows use"
    echo "       tests\\guard_page.cpp built by scripts/check_kernel_compile.cmd"
fi

echo
echo "== 3. real kbuild module build =="
kdir=""
for cand in "/lib/modules/$(uname -r)/build" /usr/src/linux-headers-$(uname -r) /usr/src/linux; do
    if [ -e "$cand/Makefile" ]; then
        kdir=$cand
        break
    fi
done

if [ -z "$kdir" ]; then
    # WSL2 and similar setups have no headers for the running Microsoft kernel.
    # Any installed header tree still proves kbuild can compile and link the core;
    # the resulting .ko simply will not load here (vermagic mismatch).
    for cand in $(ls -d /usr/src/linux-headers-*-* 2>/dev/null | grep -v -- "-common$" | sort -r); do
        if [ -e "$cand/Makefile" ]; then
            kdir=$cand
            echo "[note] no headers for the running kernel; building against $cand."
            echo "       This verifies compile+link of the core under kbuild, not loading."
            break
        fi
    done
fi

if [ -z "$kdir" ]; then
    echo "[SKIP] no kernel build tree found (install linux-headers-\$(uname -r));"
    echo "       step 1 already proves the no-runtime claim."
    exit 0
fi

work=$out/lendiza_kmod
mkdir -p "$work"
cp "$root/kernel-examples/linux/lendiza_kmod.c" \
   "$root/kernel-examples/linux/lendiza_kshim.cpp" \
   "$root/kernel-examples/linux/Makefile" "$work/"
mkdir -p "$work/include"
cp "$root/include/"*.h "$root/include/"*.hpp "$work/include/"

if make -C "$kdir" "M=$work" "LENDIZA_INCLUDE=$work/include" modules >/tmp/lendiza_kbuild.log 2>&1; then
    echo "[ OK ] module built against $kdir"
    ls -l "$work"/*.ko 2>/dev/null | sed 's/^/       /'
    echo "       objects: $(ls "$work"/*.o 2>/dev/null | wc -l)  (C module + C++ core)"
    nm -u "$work/lendiza_kshim.o" 2>/dev/null \
        | grep -E '_Znw|_Znam|__cxa_|_Unwind_|_ZTIN|_ZTSN' \
        && { echo "[FAIL] module object references the C++ runtime"; exit 1; }
    echo "[ OK ] lendiza_kshim.o references no C++ runtime symbol"
else
    echo "[FAIL] kbuild failed; last lines of /tmp/lendiza_kbuild.log:"
    tail -30 /tmp/lendiza_kbuild.log
    exit 1
fi

echo
echo "[PASS] kernel-side compile checks"
