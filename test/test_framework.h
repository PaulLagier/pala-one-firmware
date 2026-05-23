// Minimal doctest-style test framework. Single header.
// Define TEST_RUNNER_MAIN in exactly one .cpp before including this header to
// generate the `main()` that runs all registered tests.
#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>

struct TestCase {
  const char* name;
  void (*fn)();
};

inline std::vector<TestCase>& tf_allTests() {
  static std::vector<TestCase> v;
  return v;
}

struct TestRegistrar {
  TestRegistrar(const char* name, void (*fn)()) {
    tf_allTests().push_back({name, fn});
  }
};

inline int& tf_currentFailures() {
  static int n = 0;
  return n;
}

inline int& tf_totalFailures() {
  static int n = 0;
  return n;
}

inline int& tf_totalChecks() {
  static int n = 0;
  return n;
}

#define TF_CONCAT_INNER(a, b) a##b
#define TF_CONCAT(a, b) TF_CONCAT_INNER(a, b)

#define TEST_CASE(NAME)                                                              \
  static void TF_CONCAT(tf_test_, __LINE__)();                                       \
  static TestRegistrar TF_CONCAT(tf_reg_, __LINE__){NAME, &TF_CONCAT(tf_test_, __LINE__)}; \
  static void TF_CONCAT(tf_test_, __LINE__)()

#define CHECK(EXPR)                                                                  \
  do {                                                                               \
    ++tf_totalChecks();                                                              \
    if (!(EXPR)) {                                                                   \
      ++tf_currentFailures();                                                        \
      std::fprintf(stderr, "  FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #EXPR);  \
    }                                                                                \
  } while (0)

#define CHECK_EQ(A, B)                                                               \
  do {                                                                               \
    ++tf_totalChecks();                                                              \
    auto _a = (A);                                                                   \
    auto _b = (B);                                                                   \
    if (!(_a == _b)) {                                                               \
      ++tf_currentFailures();                                                        \
      std::ostringstream _oss;                                                       \
      _oss << "  FAIL " << __FILE__ << ":" << __LINE__                               \
           << "  CHECK_EQ(" << #A << ", " << #B << ")\n"                             \
           << "    lhs = " << _a << "\n"                                             \
           << "    rhs = " << _b << "\n";                                            \
      std::fputs(_oss.str().c_str(), stderr);                                        \
    }                                                                                \
  } while (0)

#define REQUIRE(EXPR)                                                                \
  do {                                                                               \
    ++tf_totalChecks();                                                              \
    if (!(EXPR)) {                                                                   \
      ++tf_currentFailures();                                                        \
      std::fprintf(stderr, "  FAIL %s:%d  REQUIRE(%s) — aborting test\n",            \
                   __FILE__, __LINE__, #EXPR);                                       \
      return;                                                                        \
    }                                                                                \
  } while (0)

#ifdef TEST_RUNNER_MAIN
int main() {
  int passed = 0, failed = 0;
  for (auto& tc : tf_allTests()) {
    tf_currentFailures() = 0;
    std::printf("[ RUN  ] %s\n", tc.name);
    tc.fn();
    if (tf_currentFailures() == 0) {
      std::printf("[  OK  ] %s\n", tc.name);
      ++passed;
    } else {
      std::printf("[ FAIL ] %s (%d failures)\n", tc.name, tf_currentFailures());
      tf_totalFailures() += tf_currentFailures();
      ++failed;
    }
  }
  std::printf("\n%zu tests, %d checks, %d passed, %d failed\n",
              tf_allTests().size(), tf_totalChecks(), passed, failed);
  return failed == 0 ? 0 : 1;
}
#endif
