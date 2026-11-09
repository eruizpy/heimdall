#include "face_expr.h"

#include <stddef.h>
#include <string.h>

static const face_expression_t kExpressions[] = {
    {"neutral relaxed cool confident", 0x22D3EE, {70, 96, 0, 0, 0}, 0, 0, false},
    {"happy laughing funny delicious silly", 0x22D3EE, {74, 96, 0, 42, 0}, 0, 0, false},
    {"sad crying", 0x60A5FA, {70, 86, 30, 0, -18}, 0, 0.4f, false},
    {"angry", 0xF43F5E, {72, 90, 34, 0, 22}, 0, 0, false},
    {"surprised shocked", 0x22D3EE, {84, 106, 0, 0, 0}, 0, 0, false},
    {"thinking confused", 0x22D3EE, {66, 88, 18, 0, 0}, -0.7f, -0.7f, false},
    {"sleepy", 0x22D3EE, {72, 96, 64, 0, 0}, 0, 0.3f, false},
    {"loving kissy", 0xF472B6, {72, 92, 0, 30, 0}, 0, 0, false},
    {"winking", 0x22D3EE, {72, 96, 0, 30, 0}, 0, 0, true},
    {"embarrassed", 0xFB7185, {70, 88, 0, 26, 0}, 0, 0.6f, false},
};

// Whole-word search in a space separated list.
static bool has_word(const char* list, const char* word) {
    size_t n = strlen(word);
    if (n == 0) {
        return false;  // strstr(p, "") never advances: an empty word would loop forever
    }
    for (const char* p = list; (p = strstr(p, word)) != NULL; p += n) {
        bool start = p == list || p[-1] == ' ';
        bool end = p[n] == '\0' || p[n] == ' ';
        if (start && end) {
            return true;
        }
    }
    return false;
}

const face_expression_t* face_expr_neutral(void) { return &kExpressions[0]; }

const face_expression_t* face_expr_find(const char* name) {
    for (size_t i = 0; i < sizeof(kExpressions) / sizeof(kExpressions[0]); i++) {
        if (name && has_word(kExpressions[i].names, name)) {
            return &kExpressions[i];
        }
    }
    return face_expr_neutral();
}
