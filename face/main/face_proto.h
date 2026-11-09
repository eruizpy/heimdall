#pragma once

// Core <-> face line protocol (docs/heimdall/face-protocol.md, spec 002-robot-face). Pure logic.

#include <stdbool.h>
#include <stddef.h>

#define FACE_PROTO_MAX_TEXT 200  // bytes of UTF-8 kept from text fields
#define FACE_PROTO_MAX_ROLE 15

typedef enum {
    FACE_CMD_PING,
    FACE_CMD_EMO,
    FACE_CMD_STATUS,
    FACE_CMD_TEXT,
    FACE_CMD_LOOK,
    FACE_CMD_BLINK,
    FACE_CMD_BRIGHT,
    FACE_CMD_CALIBRATE,
} face_cmd_type_t;

typedef struct {
    face_cmd_type_t type;
    char text[FACE_PROTO_MAX_TEXT + 1];  // emo / status / text value ("" if absent)
    char role[FACE_PROTO_MAX_ROLE + 1];  // text: assistant | user | system
    float x, y;                          // look: -1..1
    int value;                           // bright: 0..100
} face_cmd_t;

// Parses one JSON line. Returns false for invalid JSON, a missing "t" or an unknown command.
bool face_proto_parse(const char* line, face_cmd_t* out);

// Writes a touch event line (without newline). Returns its length, or -1 if it does not fit.
int face_proto_format_touch(char* buf, size_t size, const char* ev, int x, int y, int raw_x,
                            int raw_y);
