#include "country.h"

#include <stdbool.h>
#include <stdio.h>

static int total = 0;
static int failures = 0;

static void report_result(const char *label, bool pass) {
  total++;
  if (pass) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s\n", label);
    failures++;
  }
}

static void expect_valid(const char *code, const char *label) {
  report_result(label, country_is_valid_alpha2(code));
}

static void expect_invalid(const char *code, const char *label) {
  report_result(label, !country_is_valid_alpha2(code));
}

static void test_valid_codes(void) {
  printf("test_valid_codes\n");
  expect_valid("ID", "uppercase ID is valid");
  expect_valid("id", "lowercase id is valid");
  expect_valid("iD", "mixed-case iD is valid");
  expect_valid("US", "US is valid");
  expect_valid("ZW", "ZW is valid");
}

static void test_boundary_codes(void) {
  printf("test_boundary_codes\n");
  expect_valid("AD", "first table entry AD is valid");
  expect_valid("ZW", "last table entry ZW is valid");
}

static void test_invalid_codes(void) {
  printf("test_invalid_codes\n");
  expect_invalid("XX", "unknown XX is invalid");
  expect_invalid("I", "too-short I is invalid");
  expect_invalid("IDN", "too-long IDN is invalid");
  expect_invalid("1D", "non-letter 1D is invalid");
  expect_invalid("", "empty string is invalid");
  expect_invalid(NULL, "NULL is invalid");
}

int main(void) {
  printf("=== country tests ===\n\n");

  test_valid_codes();
  test_boundary_codes();
  test_invalid_codes();

  printf("\n%d/%d tests passed\n", total - failures, total);
  return failures > 0 ? 1 : 0;
}
