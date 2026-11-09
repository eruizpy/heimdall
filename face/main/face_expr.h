#pragma once

// Eye expressions keyed by the core's emotion names (spec 002-robot-face, RF3). Pure logic.

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float w, h;
    float top;     // px of the eye covered by the upper lid
    float bottom;  // px covered from below by a round lid (happy "^ ^" eyes)
    float angle;   // upper lid slant in degrees: > 0 angry (inner side down), < 0 sad
} face_shape_t;

typedef struct {
    const char* names;  // emotion names, space separated
    uint32_t color;
    face_shape_t shape;
    float look_x, look_y;  // -1..1
    bool wink;
} face_expression_t;

// Expression for an emotion name; unknown, empty or NULL names give the neutral one.
const face_expression_t* face_expr_find(const char* name);
const face_expression_t* face_expr_neutral(void);
