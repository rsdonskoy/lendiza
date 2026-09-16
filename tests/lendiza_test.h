// lendiza_test.h - dependency-free assertion harness with a GoogleTest-shaped
// API (TEST / EXPECT_EQ / --gtest-style filtering), so the suites run on a bare
// toolchain and can be moved to real GoogleTest by swapping this include.
#ifndef LENDIZA_TEST_H
#define LENDIZA_TEST_H

#include <cstdio>
#include <cstring>
#include <vector>

namespace lendiza_test {

struct TestCase {
    const char* suite;
    const char* name;
    void (*fn)();
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> r;
    return r;
}

inline int& failures()
{
    static int f = 0;
    return f;
}

inline int& checks()
{
    static int c = 0;
    return c;
}

struct Registrar {
    Registrar(const char* suite, const char* name, void (*fn)())
    {
        registry().push_back(TestCase { suite, name, fn });
    }
};

inline void fail(const char* file, int line, const char* expr)
{
    ++failures();
    std::printf("    FAIL %s:%d: %s\n", file, line, expr);
}

inline int run(int argc, char** argv)
{
    const char* filter = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--filter=", 9) == 0) {
            filter = argv[i] + 9;
        }
    }

    size_t run_count = 0;
    int failed_cases = 0;
    for (const TestCase& tc : registry()) {
        if (filter != nullptr && std::strstr(tc.suite, filter) == nullptr &&
            std::strstr(tc.name, filter) == nullptr) {
            continue;
        }
        const int before = failures();
        std::printf("[ RUN      ] %s.%s\n", tc.suite, tc.name);
        tc.fn();
        ++run_count;
        if (failures() != before) {
            ++failed_cases;
            std::printf("[  FAILED  ] %s.%s\n", tc.suite, tc.name);
        } else {
            std::printf("[       OK ] %s.%s\n", tc.suite, tc.name);
        }
    }

    std::printf("[==========] %zu tests ran, %d failed, %d checks\n", run_count, failed_cases, checks());
    return failed_cases == 0 ? 0 : 1;
}

} // namespace lendiza_test

#define TEST(suite, name)                                                                              \
    static void suite##_##name##_body();                                                               \
    static lendiza_test::Registrar suite##_##name##_reg(#suite, #name, &suite##_##name##_body);         \
    static void suite##_##name##_body()

#define EXPECT_EQ(actual, expected)                                                                    \
    do {                                                                                               \
        ++lendiza_test::checks();                                                                      \
        const auto lz_a = (actual);                                                                    \
        const auto lz_e = (expected);                                                                  \
        if (lz_a != lz_e) {                                                                            \
            std::printf("    %s:%d: expected %lld, got %lld  (%s == %s)\n", __FILE__, __LINE__,         \
                        static_cast<long long>(lz_e), static_cast<long long>(lz_a), #actual, #expected); \
            lendiza_test::fail(__FILE__, __LINE__, #actual " == " #expected);                           \
        }                                                                                              \
    } while (0)

#define EXPECT_TRUE(cond)                                                                              \
    do {                                                                                               \
        ++lendiza_test::checks();                                                                      \
        if (!(cond)) {                                                                                 \
            lendiza_test::fail(__FILE__, __LINE__, #cond);                                             \
        }                                                                                              \
    } while (0)

#define LENDIZA_TEST_MAIN()                                                                            \
    int main(int argc, char** argv)                                                                    \
    {                                                                                                  \
        return lendiza_test::run(argc, argv);                                                           \
    }

#endif // LENDIZA_TEST_H
