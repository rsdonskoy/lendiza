// guard_page.cpp - proves the decoder never reads past the bytes it was given.
//
// In a driver an over-read is a bugcheck, not a wrong number, so this program
// makes the claim physical: it places each input so that its last byte sits
// directly against an inaccessible page, then decodes.  Any read beyond the
// buffer faults, and the fault is turned into a reported violation instead of a
// crash.
//
// It is deliberately dependency-free (no CRT assumptions inside the decoder, no
// test framework around it) and runs on both Windows and Linux:
//   cl /std:c++17 /EHsc /O2 guard_page.cpp ../src/lendiza.cpp /Fe:guard_page.exe
//   g++ -std=c++17 -O1 -I../include guard_page.cpp ../src/lendiza.cpp -o guard_page
// Exit code 0 means no over-read was observed.

#include <csetjmp>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "lendiza_code_corpus.h"
#include "lendiza_core.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace {

std::jmp_buf g_fault_jump;
volatile bool g_in_tracked_call = false;
volatile size_t g_fault_offset = 0;

const uint8_t* g_input = nullptr;
size_t g_input_len = 0;
size_t g_violations = 0;
size_t g_cases = 0;

/* The last byte of the window is placed flush against the inaccessible page. */
class GuardedWindow {
public:
    GuardedWindow()
    {
#if defined(_WIN32)
        SYSTEM_INFO si {};
        GetSystemInfo(&si);
        page_ = si.dwPageSize;
        base_ = static_cast<uint8_t*>(
            VirtualAlloc(nullptr, page_ * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (base_ == nullptr) {
            std::fprintf(stderr, "VirtualAlloc failed\n");
            return;
        }
        DWORD old = 0;
        if (!VirtualProtect(base_ + page_, page_, PAGE_NOACCESS, &old)) {
            std::fprintf(stderr, "VirtualProtect(PAGE_NOACCESS) failed\n");
            base_ = nullptr;
            return;
        }
#else
        page_ = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        void* p = mmap(nullptr, page_ * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                       0);
        if (p == MAP_FAILED) {
            std::fprintf(stderr, "mmap failed\n");
            return;
        }
        if (mprotect(static_cast<uint8_t*>(p) + page_, page_, PROT_NONE) != 0) {
            std::fprintf(stderr, "mprotect(PROT_NONE) failed\n");
            return;
        }
        base_ = static_cast<uint8_t*>(p);
#endif
        ok_ = true;
    }

    ~GuardedWindow()
    {
        if (base_ == nullptr) {
            return;
        }
#if defined(_WIN32)
        VirtualFree(base_, 0, MEM_RELEASE);
#else
        munmap(base_, page_ * 2);
#endif
    }

    bool ok() const { return ok_; }

    /* Copies len bytes so that the final byte is the last readable one. */
    const uint8_t* place(const uint8_t* src, size_t len)
    {
        if (!ok_ || len == 0 || len > page_) {
            return nullptr;
        }
        uint8_t* start = base_ + page_ - len;
        std::memcpy(start, src, len);
        return start;
    }

private:
    uint8_t* base_ = nullptr;
    size_t page_ = 0;
    bool ok_ = false;
};

#if defined(_WIN32)
LONG CALLBACK on_fault(EXCEPTION_POINTERS* info)
{
    if (!g_in_tracked_call) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code == STATUS_ACCESS_VIOLATION || code == STATUS_GUARD_PAGE_VIOLATION) {
        std::longjmp(g_fault_jump, 1);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
void on_fault(int, siginfo_t* info, void*)
{
    if (!g_in_tracked_call) {
        std::fprintf(stderr, "untracked fault, aborting\n");
        std::exit(2);
    }
    g_fault_offset = reinterpret_cast<uintptr_t>(info->si_addr);
    std::longjmp(g_fault_jump, 1);
}
#endif

void install_handler()
{
#if defined(_WIN32)
    AddVectoredExceptionHandler(1, &on_fault);
#else
    struct sigaction sa {};
    sa.sa_sigaction = &on_fault;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
#endif
}

/* Decodes at the edge of the guard page; true if a fault was taken. */
bool probe(GuardedWindow& win, const uint8_t* src, size_t len, bool long_mode, size_t* result)
{
    const uint8_t* placed = win.place(src, len);
    if (placed == nullptr) {
        return false;
    }
    if (setjmp(g_fault_jump) != 0) {
        g_in_tracked_call = false;
        return true;
    }
    g_in_tracked_call = true;
    *result = long_mode ? lendiza::detail::amd64traits::ldiza(placed, len)
                        : lendiza::detail::x86traits::ldiza(placed, len);
    g_in_tracked_call = false;
    return false;
}

/* Negative control: deliberately read one byte past the guarded window.  If that
 * does not fault, the guard page or the handler is not in effect and every "0
 * over-reads" result below would be worthless. */
bool self_test(GuardedWindow& win)
{
    static const uint8_t src[4] = { 0x90, 0x90, 0x90, 0xC3 };
    const uint8_t* placed = win.place(src, sizeof(src));
    if (placed == nullptr) {
        return false;
    }
    if (setjmp(g_fault_jump) != 0) {
        g_in_tracked_call = false;
        return true;
    }
    g_in_tracked_call = true;
    volatile const uint8_t* walk = placed;
    volatile uint8_t sink = walk[sizeof(src)]; /* one past the last readable byte */
    (void)sink;
    g_in_tracked_call = false;
    return false;
}

void report(const char* what, size_t len)
{
    ++g_violations;
    if (g_violations < 20) {
        std::printf("  OVER-READ %s n=%zu bytes=", what, len);
        for (size_t i = 0; i < g_input_len && i < 16; ++i) {
            std::printf("%02X", g_input[i]);
        }
        std::printf("\n");
    }
}

/* Runs one input through both modes at every truncation length. */
void check_input(GuardedWindow& win, const uint8_t* src, size_t len)
{
    for (size_t n = 1; n <= len && n <= LDZ_MAX_INSNS_LEN; ++n) {
        for (const bool long_mode : { false, true }) {
            g_input = src;
            g_input_len = n;
            ++g_cases;
            size_t result = 0;
            if (probe(win, src, n, long_mode, &result)) {
                report(long_mode ? "x64" : "x86", n);
                continue;
            }
            /* A returned length must fit inside what we handed over. */
            if (result < 0xE0 && result > n) {
                report(long_mode ? "x64-guessed" : "x86-guessed", n);
            }
        }
    }
}

} // namespace

int main()
{
    GuardedWindow win;
    if (!win.ok()) {
        return 2;
    }
    install_handler();

    if (!self_test(win)) {
        std::printf("SELF-TEST FAILED: a deliberate read past the guarded window did not fault;\n"
                    "                  the harness cannot prove anything about over-reads.\n");
        return 3;
    }
    std::printf("self-test: an intentional over-read faults as expected\n");

    /* 1. Every one-, two- and three-byte sequence over a wide alphabet. */
    static const uint8_t alphabet[] = { 0x00, 0x04, 0x05, 0x0D, 0x0F, 0x24, 0x25, 0x2D, 0x36, 0x3A,
                                        0x3D, 0x40, 0x41, 0x48, 0x4B, 0x62, 0x63, 0x66, 0x67, 0x6A,
                                        0x6B, 0x71, 0x75, 0x7F, 0x80, 0x83, 0x84, 0x8D, 0xA1, 0xA5,
                                       0xB1, 0xB8, 0xC1, 0xC4, 0xC5, 0xC7, 0xCC, 0xD0, 0xD8, 0xDF,
                                        0xE3, 0xE8, 0xEA, 0xF2, 0xF6, 0xF7, 0xFA, 0xFF };
    const size_t an = sizeof(alphabet) / sizeof(alphabet[0]);
    uint8_t buf[3];
    for (size_t i = 0; i < an; ++i) {
        buf[0] = alphabet[i];
        check_input(win, buf, 1);
        for (size_t j = 0; j < an; ++j) {
            buf[1] = alphabet[j];
            check_input(win, buf, 2);
            for (size_t k = 0; k < an; ++k) {
                buf[2] = alphabet[k];
                check_input(win, buf, 3);
            }
        }
    }

    /* 2. Real code: every offset, every window size. */
    size_t n = 0;
    const uint8_t* code = lzc::shellcode(n);
    for (size_t off = 0; off < n; ++off) {
        const size_t avail = (n - off < LDZ_MAX_INSNS_LEN) ? (n - off) : LDZ_MAX_INSNS_LEN;
        check_input(win, code + off, avail);
    }

    /* 3. Exhaustive two-byte opcode prefixes with a long tail, so every dispatch
     * path is probed at every truncation depth. */
    uint8_t wide[15];
    for (uint32_t a = 0; a < 256; ++a) {
        for (uint32_t b = 0; b < 256; ++b) {
            wide[0] = static_cast<uint8_t>(a);
            wide[1] = static_cast<uint8_t>(b);
            for (size_t k = 2; k < 15; ++k) {
                wide[k] = static_cast<uint8_t>((k % 2) ? 0x25 : 0x05);
            }
            check_input(win, wide, 15);
        }
    }

    std::printf("guard-page probe: %zu decodes at the edge of an inaccessible page\n", g_cases);
    std::printf("over-reads: %zu\n", g_violations);
    return g_violations == 0 ? 0 : 1;
}
