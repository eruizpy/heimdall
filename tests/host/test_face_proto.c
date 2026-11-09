// Spec 002-robot-face, CA1 and CA2: protocol parsing and event formatting.
#include <string.h>

#include "cJSON.h"
#include "face_proto.h"
#include "unity.h"

static face_cmd_t cmd;

void setUp(void) {
    memset(&cmd, 0xAB, sizeof(cmd));  // garbage, so missing fields are caught
}

void tearDown(void) {}

static void test_emo_reads_value(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"emo\",\"v\":\"happy\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_EMO, cmd.type);
    TEST_ASSERT_EQUAL_STRING("happy", cmd.text);
}

static void test_emo_without_value_is_empty(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"emo\"}", &cmd));
    TEST_ASSERT_EQUAL_STRING("", cmd.text);
}

static void test_status_reads_value(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"status\",\"v\":\"Escuchando\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_STATUS, cmd.type);
    TEST_ASSERT_EQUAL_STRING("Escuchando", cmd.text);
}

static void test_text_keeps_utf8_and_role(void) {
    TEST_ASSERT_TRUE(
        face_proto_parse("{\"t\":\"text\",\"role\":\"user\",\"v\":\"¿Cómo estás, Ñandú?\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_TEXT, cmd.type);
    TEST_ASSERT_EQUAL_STRING("user", cmd.role);
    TEST_ASSERT_EQUAL_STRING("¿Cómo estás, Ñandú?", cmd.text);
}

static void test_text_role_defaults_to_assistant(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"text\",\"v\":\"Hola\"}", &cmd));
    TEST_ASSERT_EQUAL_STRING("assistant", cmd.role);
}

static void test_text_unknown_role_falls_back_to_assistant(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"text\",\"role\":\"hacker\",\"v\":\"x\"}", &cmd));
    TEST_ASSERT_EQUAL_STRING("assistant", cmd.role);
}

static void test_long_text_is_truncated_without_overflow(void) {
    char line[600] = "{\"t\":\"text\",\"v\":\"";
    size_t len = strlen(line);
    memset(line + len, 'a', 400);
    strcpy(line + len + 400, "\"}");
    TEST_ASSERT_TRUE(face_proto_parse(line, &cmd));
    TEST_ASSERT_EQUAL_size_t(FACE_PROTO_MAX_TEXT, strlen(cmd.text));
}

static void test_truncation_does_not_split_utf8(void) {
    // 'ñ' is 2 bytes; with 199 ASCII chars before it, a byte cut at 200 would split it.
    char line[600] = "{\"t\":\"text\",\"v\":\"";
    size_t len = strlen(line);
    memset(line + len, 'a', FACE_PROTO_MAX_TEXT - 1);
    strcpy(line + len + FACE_PROTO_MAX_TEXT - 1, "ñññ\"}");
    TEST_ASSERT_TRUE(face_proto_parse(line, &cmd));
    TEST_ASSERT_EQUAL_size_t(FACE_PROTO_MAX_TEXT - 1, strlen(cmd.text));
}

static void test_look_reads_and_clamps(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"look\",\"x\":-0.5,\"y\":0.25}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_LOOK, cmd.type);
    TEST_ASSERT_EQUAL_FLOAT(-0.5f, cmd.x);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, cmd.y);

    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"look\",\"x\":7,\"y\":-3}", &cmd));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, cmd.x);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, cmd.y);
}

static void test_look_missing_axes_default_to_center(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"look\"}", &cmd));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cmd.x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cmd.y);
}

static void test_bright_reads_and_clamps(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"bright\",\"v\":40}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_BRIGHT, cmd.type);
    TEST_ASSERT_EQUAL_INT(40, cmd.value);
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"bright\",\"v\":250}", &cmd));
    TEST_ASSERT_EQUAL_INT(100, cmd.value);
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"bright\",\"v\":-5}", &cmd));
    TEST_ASSERT_EQUAL_INT(0, cmd.value);
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"bright\"}", &cmd));
    TEST_ASSERT_EQUAL_INT(80, cmd.value);
}

static void test_commands_without_fields(void) {
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"ping\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_PING, cmd.type);
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"blink\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_BLINK, cmd.type);
    TEST_ASSERT_TRUE(face_proto_parse("{\"t\":\"calibrate\"}", &cmd));
    TEST_ASSERT_EQUAL(FACE_CMD_CALIBRATE, cmd.type);
}

static void test_rejects_invalid_input(void) {
    TEST_ASSERT_FALSE(face_proto_parse(NULL, &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("I (1234) boot: log line", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("{\"t\":\"emo\"", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("{\"v\":\"happy\"}", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("{\"t\":42}", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("{\"t\":\"selfdestruct\"}", &cmd));
    TEST_ASSERT_FALSE(face_proto_parse("[1,2,3]", &cmd));
}

static void test_format_touch_is_valid_json(void) {
    char buf[128];
    int n = face_proto_format_touch(buf, sizeof(buf), "tap", 201, 142, 2494, 2430);
    TEST_ASSERT_EQUAL_STRING(
        "{\"t\":\"touch\",\"ev\":\"tap\",\"x\":201,\"y\":142,\"raw\":[2494,2430]}", buf);
    TEST_ASSERT_EQUAL_INT((int)strlen(buf), n);
    cJSON* root = cJSON_Parse(buf);
    TEST_ASSERT_NOT_NULL(root);
    cJSON_Delete(root);
}

static void test_format_touch_reports_small_buffer(void) {
    char buf[16];
    TEST_ASSERT_EQUAL_INT(-1, face_proto_format_touch(buf, sizeof(buf), "swipe_left", 1, 2, 3, 4));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_emo_reads_value);
    RUN_TEST(test_emo_without_value_is_empty);
    RUN_TEST(test_status_reads_value);
    RUN_TEST(test_text_keeps_utf8_and_role);
    RUN_TEST(test_text_role_defaults_to_assistant);
    RUN_TEST(test_text_unknown_role_falls_back_to_assistant);
    RUN_TEST(test_long_text_is_truncated_without_overflow);
    RUN_TEST(test_truncation_does_not_split_utf8);
    RUN_TEST(test_look_reads_and_clamps);
    RUN_TEST(test_look_missing_axes_default_to_center);
    RUN_TEST(test_bright_reads_and_clamps);
    RUN_TEST(test_commands_without_fields);
    RUN_TEST(test_rejects_invalid_input);
    RUN_TEST(test_format_touch_is_valid_json);
    RUN_TEST(test_format_touch_reports_small_buffer);
    return UNITY_END();
}
