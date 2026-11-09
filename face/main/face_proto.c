#include "face_proto.h"

#include <stdio.h>
#include <string.h>

#include "cJSON.h"

static const struct {
    const char* name;
    face_cmd_type_t type;
} kCommands[] = {
    {"ping", FACE_CMD_PING},     {"emo", FACE_CMD_EMO},
    {"status", FACE_CMD_STATUS}, {"text", FACE_CMD_TEXT},
    {"look", FACE_CMD_LOOK},     {"blink", FACE_CMD_BLINK},
    {"bright", FACE_CMD_BRIGHT}, {"calibrate", FACE_CMD_CALIBRATE},
};

static float clampf(double v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : (float)v); }

// Copies at most `max` bytes without cutting a UTF-8 sequence in half.
static void copy_utf8(char* dst, const char* src, size_t max) {
    size_t len = strlen(src);
    if (len > max) {
        len = max;
        while (len > 0 && ((unsigned char)src[len] & 0xC0) == 0x80) {
            len--;  // src[len] is a continuation byte: back up to the start of its sequence
        }
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static const char* str_field(const cJSON* root, const char* key) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    return cJSON_IsString(item) ? item->valuestring : NULL;
}

static double num_field(const cJSON* root, const char* key, double fallback) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, key);
    return cJSON_IsNumber(item) ? item->valuedouble : fallback;
}

bool face_proto_parse(const char* line, face_cmd_t* out) {
    if (line == NULL || out == NULL) {
        return false;
    }
    cJSON* root = cJSON_Parse(line);
    if (!cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return false;
    }
    const char* t = str_field(root, "t");
    bool known = false;
    for (size_t i = 0; t != NULL && i < sizeof(kCommands) / sizeof(kCommands[0]); i++) {
        if (strcmp(t, kCommands[i].name) == 0) {
            out->type = kCommands[i].type;
            known = true;
            break;
        }
    }
    if (!known) {
        cJSON_Delete(root);
        return false;
    }

    const char* v = str_field(root, "v");
    copy_utf8(out->text, v ? v : "", FACE_PROTO_MAX_TEXT);
    const char* role = str_field(root, "role");
    if (role == NULL || (strcmp(role, "user") != 0 && strcmp(role, "system") != 0)) {
        role = "assistant";
    }
    copy_utf8(out->role, role, FACE_PROTO_MAX_ROLE);
    out->x = clampf(num_field(root, "x", 0), -1.0f, 1.0f);
    out->y = clampf(num_field(root, "y", 0), -1.0f, 1.0f);
    out->value = (int)clampf(num_field(root, "v", 80), 0.0f, 100.0f);

    cJSON_Delete(root);
    return true;
}

int face_proto_format_touch(char* buf, size_t size, const char* ev, int x, int y, int raw_x,
                            int raw_y) {
    int n = snprintf(buf, size, "{\"t\":\"touch\",\"ev\":\"%s\",\"x\":%d,\"y\":%d,\"raw\":[%d,%d]}",
                     ev, x, y, raw_x, raw_y);
    return (n < 0 || (size_t)n >= size) ? -1 : n;
}
