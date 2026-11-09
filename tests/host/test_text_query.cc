// Spec 001-text-query, CA1: text query sanitizing.
#include "text_query.h"

#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_plain_text_is_unchanged(void) {
    TEST_ASSERT_EQUAL_STRING("Contame un dato curioso",
                             SanitizeTextQuery("Contame un dato curioso").c_str());
}

static void test_keeps_utf8(void) {
    TEST_ASSERT_EQUAL_STRING("¿Qué hora es, Ñandú?",
                             SanitizeTextQuery("¿Qué hora es, Ñandú?").c_str());
}

static void test_removes_json_breaking_characters(void) {
    TEST_ASSERT_EQUAL_STRING("dijo hola", SanitizeTextQuery("dijo \"hola\"").c_str());
    TEST_ASSERT_EQUAL_STRING("a b", SanitizeTextQuery("a\\ b").c_str());
}

static void test_removes_control_characters(void) {
    TEST_ASSERT_EQUAL_STRING("unodos", SanitizeTextQuery("uno\n\tdos\r").c_str());
}

static void test_trims_surrounding_spaces(void) {
    TEST_ASSERT_EQUAL_STRING("hola", SanitizeTextQuery("   hola  ").c_str());
}

static void test_blank_input_becomes_empty(void) {
    TEST_ASSERT_EQUAL_STRING("", SanitizeTextQuery("").c_str());
    TEST_ASSERT_EQUAL_STRING("", SanitizeTextQuery("  \"\" \n ").c_str());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_plain_text_is_unchanged);
    RUN_TEST(test_keeps_utf8);
    RUN_TEST(test_removes_json_breaking_characters);
    RUN_TEST(test_removes_control_characters);
    RUN_TEST(test_trims_surrounding_spaces);
    RUN_TEST(test_blank_input_becomes_empty);
    return UNITY_END();
}
