// Spec 002-robot-face, CA3: emotion name -> expression.
#include "face_expr.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_neutral_is_the_default(void) {
    const face_expression_t* neutral = face_expr_neutral();
    TEST_ASSERT_NOT_NULL(neutral);
    TEST_ASSERT_EQUAL_PTR(neutral, face_expr_find("neutral"));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, neutral->shape.top);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, neutral->shape.bottom);
}

static void test_synonyms_share_an_expression(void) {
    TEST_ASSERT_EQUAL_PTR(face_expr_find("happy"), face_expr_find("laughing"));
    TEST_ASSERT_EQUAL_PTR(face_expr_find("sad"), face_expr_find("crying"));
    TEST_ASSERT_EQUAL_PTR(face_expr_find("surprised"), face_expr_find("shocked"));
    TEST_ASSERT_EQUAL_PTR(face_expr_find("neutral"), face_expr_find("relaxed"));
}

static void test_shapes_express_the_emotion(void) {
    TEST_ASSERT_TRUE(face_expr_find("happy")->shape.bottom > 0);  // "^ ^" eyes
    TEST_ASSERT_TRUE(face_expr_find("angry")->shape.angle > 0);   // inner side down
    TEST_ASSERT_TRUE(face_expr_find("sad")->shape.angle < 0);     // outer side down
    TEST_ASSERT_TRUE(face_expr_find("sleepy")->shape.top > face_expr_find("neutral")->shape.h / 2);
    TEST_ASSERT_TRUE(face_expr_find("surprised")->shape.h > face_expr_find("neutral")->shape.h);
    TEST_ASSERT_TRUE(face_expr_find("winking")->wink);
    TEST_ASSERT_FALSE(face_expr_find("happy")->wink);
}

static void test_unknown_names_fall_back_to_neutral(void) {
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find("xyz"));
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find(NULL));
}

static void test_empty_name_falls_back_to_neutral(void) {
    // Regression: an empty name used to loop forever in the word search.
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find(""));
}

static void test_matches_whole_words_only(void) {
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find("app"));     // inside "happy"
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find("happy "));  // trailing space
    TEST_ASSERT_EQUAL_PTR(face_expr_neutral(), face_expr_find("ad"));      // inside "sad"
}

static void test_every_expression_has_a_visible_eye(void) {
    const char* names[] = {"neutral",  "happy",  "sad",    "angry",   "surprised",
                           "thinking", "sleepy", "loving", "winking", "embarrassed"};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        const face_expression_t* e = face_expr_find(names[i]);
        if (i > 0) {
            TEST_ASSERT_TRUE_MESSAGE(e != face_expr_neutral(), names[i]);
        }
        TEST_ASSERT_TRUE_MESSAGE(e->shape.w > 0 && e->shape.h > 0, names[i]);
        TEST_ASSERT_TRUE_MESSAGE(e->shape.top + e->shape.bottom < e->shape.h, names[i]);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_neutral_is_the_default);
    RUN_TEST(test_synonyms_share_an_expression);
    RUN_TEST(test_shapes_express_the_emotion);
    RUN_TEST(test_unknown_names_fall_back_to_neutral);
    RUN_TEST(test_empty_name_falls_back_to_neutral);
    RUN_TEST(test_matches_whole_words_only);
    RUN_TEST(test_every_expression_has_a_visible_eye);
    return UNITY_END();
}
