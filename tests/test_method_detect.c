#include "method_detect.h"
#include <stdio.h>
#include <string.h>

static int total = 0;
static int failures = 0;

static void expect_method(const char *country_code, CalcMethod expected, const char *label) {
  total++;
  CalcMethod got = method_detect_by_country(country_code);
  if (got == expected) {
    printf("  PASS: %s\n", label);
  } else {
    printf("  FAIL: %s (expected %d, got %d)\n", label, expected, got);
    failures++;
  }
}

static void test_specific_countries(void) {
  printf("test_specific_countries\n");
  expect_method("ID", CALC_KEMENAG, "Indonesia -> kemenag");
  expect_method("MY", CALC_JAKIM, "Malaysia -> jakim");
  expect_method("SG", CALC_SINGAPORE, "Singapore -> singapore");
  expect_method("SA", CALC_MAKKAH, "Saudi Arabia -> makkah");
  expect_method("YE", CALC_MAKKAH, "Yemen -> makkah");
  expect_method("US", CALC_ISNA, "United States -> isna");
  expect_method("CA", CALC_ISNA, "Canada -> isna");
  expect_method("EG", CALC_EGYPT, "Egypt -> egypt");
  expect_method("LY", CALC_EGYPT, "Libya -> egypt");
  expect_method("SD", CALC_EGYPT, "Sudan -> egypt");
  expect_method("PK", CALC_KARACHI, "Pakistan -> karachi");
  expect_method("IN", CALC_KARACHI, "India -> karachi");
  expect_method("BD", CALC_KARACHI, "Bangladesh -> karachi");
  expect_method("AF", CALC_KARACHI, "Afghanistan -> karachi");
  expect_method("TR", CALC_TURKEY, "Turkey -> turkey");
  expect_method("AE", CALC_DUBAI, "UAE -> dubai");
  expect_method("QA", CALC_QATAR, "Qatar -> qatar");
  expect_method("KW", CALC_KUWAIT, "Kuwait -> kuwait");
  expect_method("JO", CALC_JORDAN, "Jordan -> jordan");
  expect_method("PS", CALC_JORDAN, "Palestine -> jordan");
  expect_method("BH", CALC_GULF, "Bahrain -> gulf");
  expect_method("OM", CALC_GULF, "Oman -> gulf");
  expect_method("TN", CALC_TUNISIA, "Tunisia -> tunisia");
  expect_method("DZ", CALC_ALGERIA, "Algeria -> algeria");
  expect_method("MA", CALC_MOROCCO, "Morocco -> morocco");
  expect_method("PT", CALC_PORTUGAL, "Portugal -> portugal");
  expect_method("FR", CALC_FRANCE, "France -> france");
  expect_method("RU", CALC_RUSSIA, "Russia -> russia");
}

static void test_fallback_to_mwl(void) {
  printf("test_fallback_to_mwl\n");
  expect_method("DE", CALC_MWL, "Germany -> mwl (fallback)");
  expect_method("JP", CALC_MWL, "Japan -> mwl (fallback)");
  expect_method("BR", CALC_MWL, "Brazil -> mwl (fallback)");
  expect_method("XX", CALC_MWL, "Unknown code -> mwl (fallback)");
}

static void test_edge_cases(void) {
  printf("test_edge_cases\n");
  expect_method(NULL, CALC_MWL, "NULL -> mwl (fallback)");
  expect_method("", CALC_MWL, "empty string -> mwl (fallback)");
  expect_method("id", CALC_KEMENAG, "lowercase 'id' -> kemenag");
  expect_method("Id", CALC_KEMENAG, "mixed case 'Id' -> kemenag");
}

int main(void) {
  printf("=== method_detect tests ===\n\n");

  test_specific_countries();
  test_fallback_to_mwl();
  test_edge_cases();

  printf("\n%d/%d tests passed\n", total - failures, total);
  return failures > 0 ? 1 : 0;
}
