# Kernel-mode example: a kbuild module whose decode core is C++ headers.
#
#   make -C /lib/modules/$(uname -r)/build M=$PWD modules
#   sudo insmod lendiza_demo.ko                 # then: dmesg | tail
#   sudo insmod lendiza_demo.ko decode_addr=0x7ffd1234 long_mode=0
#   make -C /lib/modules/$(uname -r)/build M=$PWD clean
#
# kbuild only compiles .c, so lendiza_kshim.cpp is built by the rule below with no
# libstdc++, no CRT startup, no exceptions and no RTTI.
#
# The C++ TU is compiled *without* the kernel's include flags on purpose: kernel
# headers are not C++-clean before v6.15 (rwonce.h, `auto` as __auto_type, and
# `inline` expanded to `__gnu_inline`), and the decode core needs no kernel API.
# The C half of the module keeps the normal kernel flags and maps raw decoder
# results onto errno.  Passing LINUXINCLUDE here instead would be the thing that
# breaks, not anything in lendiza.
#
# The module target must not share the .c file's stem: kbuild generates a
# composite <target>.o, which would be a circular dependency.

obj-m += lendiza_demo.o
lendiza_demo-y := lendiza_kmod.o lendiza_kshim.o

CXX ?= g++

# Headers of the library.  Defaults to the in-tree location; pass
# LENDIZA_INCLUDE=<dir> when building from a copied-out directory.
LENDIZA_INCLUDE ?= $(src)/../../include

lendiza_cxxflags := -x c++ -std=gnu++17 -O2 -ffreestanding -nostdinc++ \
                    -fno-exceptions -fno-rtti -fno-builtin -fno-stack-protector \
                    -fno-asynchronous-unwind-tables -fno-pic -fno-strict-aliasing \
                    -Wall -Wextra -Wno-error -I$(LENDIZA_INCLUDE) \
                    -isystem $(shell $(CC) -print-file-name=include)

quiet_cmd_cxx_module = CXX  $@
      cmd_cxx_module = $(CXX) $(lendiza_cxxflags) -c $(filter %.cpp,$^) -o $@

$(obj)/lendiza_kshim.o: $(src)/lendiza_kshim.cpp FORCE
	$(call if_changed,cxx_module)
