#include "face_ui.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "board_pins.h"
#include "lvgl.h"

LV_FONT_DECLARE(font_noto_sans_basic_20_4);
LV_FONT_DECLARE(font_noto_sans_basic_16_4);

// Each eye is drawn by hand inside a box (rounded rect + black lids), which avoids LVGL
// transforms: rotated layers would need ~100 KB of RAM and this board has no PSRAM.
#define BOX_W 110
#define BOX_H 124
#define EYE_CY 98
#define EYE_DX 62
#define TICK_MS 30

typedef struct {
    float w, h;
    float top;     // px of the eye covered by the upper lid
    float bottom;  // px covered from below by a round lid (happy "^ ^" eyes)
    float angle;   // upper lid slant in degrees: > 0 angry (inner side down), < 0 sad
} shape_t;

typedef struct {
    const char* names;  // xiaozhi emotion names, space separated
    uint32_t color;
    shape_t shape;
    float look_x, look_y;  // -1..1
    bool wink;
} expression_t;

static const expression_t kExpressions[] = {
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

static lv_obj_t* s_eyes[2];
static lv_obj_t* s_status;
static lv_obj_t* s_text;
static lv_obj_t* s_calib_bg;
static lv_obj_t* s_calib_h;
static lv_obj_t* s_calib_v;
static lv_obj_t* s_calib_label;

static shape_t s_cur, s_target;
static lv_color_t s_color;
static bool s_wink;
static float s_look_x, s_look_y;    // current, px
static float s_look_tx, s_look_ty;  // target, px
static float s_expr_look_x, s_expr_look_y;
static float s_blink;  // 0 open .. 1 closed
static uint32_t s_blink_start, s_next_blink, s_next_glance, s_look_hold_until;
static face_event_cb_t s_event_cb;

static float lerpf(float a, float b, float k) { return a + (b - a) * k; }

static void draw_eye(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    lv_layer_t* layer = lv_event_get_layer(e);
    int idx = obj == s_eyes[0] ? 0 : 1;

    lv_area_t box;
    lv_obj_get_coords(obj, &box);

    float h = s_cur.h * (1.0f - s_blink);
    if (s_wink && idx == 0 && h > 8) {
        h = 8;
    }
    if (h < 6) {
        h = 6;
    }
    float scale = h / s_cur.h;
    float w = s_cur.w;
    int x = box.x1 + (int)((BOX_W - w) / 2 + s_look_x);
    int y = box.y1 + (int)((BOX_H - h) / 2 + s_look_y);

    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.bg_color = s_color;
    rect.bg_opa = LV_OPA_COVER;
    rect.radius = (int)(h / 2 < 30 ? h / 2 : 30);
    lv_area_t eye = {x, y, x + (int)w - 1, y + (int)h - 1};
    lv_draw_rect(layer, &rect, &eye);

    float top = s_cur.top * scale;
    if (top > 0.5f) {
        // Mirror the slant between eyes; positive LVGL y is down.
        float angle = idx == 0 ? s_cur.angle : -s_cur.angle;
        float dy = tanf(angle * (float)M_PI / 180.0f) * w / 2;
        int left = x - 4, right = x + (int)w + 4, sky = box.y1;
        int yl = y + (int)(top - dy), yr = y + (int)(top + dy);
        lv_draw_triangle_dsc_t tri;
        lv_draw_triangle_dsc_init(&tri);
        tri.color = lv_color_black();
        tri.opa = LV_OPA_COVER;
        tri.p[0] = (lv_point_precise_t){left, sky};
        tri.p[1] = (lv_point_precise_t){right, sky};
        tri.p[2] = (lv_point_precise_t){right, yr};
        lv_draw_triangle(layer, &tri);
        tri.p[1] = (lv_point_precise_t){right, yr};
        tri.p[2] = (lv_point_precise_t){left, yl};
        lv_draw_triangle(layer, &tri);
    }

    float bottom = s_cur.bottom * scale;
    if (bottom > 0.5f) {
        int d = (int)(w * 2), cx = x + (int)(w / 2), top_y = y + (int)(h - bottom);
        lv_draw_rect_dsc_t lid;
        lv_draw_rect_dsc_init(&lid);
        lid.bg_color = lv_color_black();
        lid.bg_opa = LV_OPA_COVER;
        lid.radius = LV_RADIUS_CIRCLE;
        lv_area_t circle = {cx - d / 2, top_y, cx + d / 2, top_y + d};
        lv_draw_rect(layer, &lid, &circle);
    }
}

static bool near(float a, float b) { return fabsf(a - b) < 0.3f; }

static void tick_cb(lv_timer_t* timer) {
    uint32_t now = lv_tick_get();
    bool changed = false;

    float* cur[] = {&s_cur.w, &s_cur.h, &s_cur.top, &s_cur.bottom, &s_cur.angle};
    float tgt[] = {s_target.w, s_target.h, s_target.top, s_target.bottom, s_target.angle};
    for (int i = 0; i < 5; i++) {
        if (!near(*cur[i], tgt[i])) {
            *cur[i] = lerpf(*cur[i], tgt[i], 0.35f);
            changed = true;
        }
    }

    // Idle glances when nobody is directing the eyes.
    if (now > s_look_hold_until && now > s_next_glance) {
        bool center = lv_rand(0, 2) == 0;
        s_look_tx = s_expr_look_x + (center ? 0 : (float)lv_rand(0, 28) - 14);
        s_look_ty = s_expr_look_y + (center ? 0 : (float)lv_rand(0, 16) - 8);
        s_next_glance = now + lv_rand(2500, 7000);
    }
    if (!near(s_look_x, s_look_tx) || !near(s_look_y, s_look_ty)) {
        s_look_x = lerpf(s_look_x, s_look_tx, 0.3f);
        s_look_y = lerpf(s_look_y, s_look_ty, 0.3f);
        changed = true;
    }

    if (s_blink_start == 0 && now > s_next_blink) {
        s_blink_start = now;
    }
    if (s_blink_start != 0) {
        uint32_t t = now - s_blink_start;
        if (t >= 180) {
            s_blink = 0;
            s_blink_start = 0;
            s_next_blink = now + lv_rand(2500, 6000);
        } else {
            s_blink = t < 90 ? t / 90.0f : 1.0f - (t - 90) / 90.0f;
        }
        changed = true;
    }

    if (changed) {
        lv_obj_invalidate(s_eyes[0]);
        lv_obj_invalidate(s_eyes[1]);
    }
}

static void emit(const char* ev, lv_point_t p) {
    if (s_event_cb) {
        s_event_cb(ev, p.x, p.y);
    }
}

static void screen_event_cb(lv_event_t* e) {
    lv_indev_t* indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    switch (lv_event_get_code(e)) {
        case LV_EVENT_PRESSED:
            // Look at the finger.
            s_look_tx = (p.x - LCD_H_RES / 2) * 18.0f / (LCD_H_RES / 2);
            s_look_ty = (p.y - LCD_V_RES / 2) * 12.0f / (LCD_V_RES / 2);
            s_look_hold_until = lv_tick_get() + 1500;
            break;
        case LV_EVENT_SHORT_CLICKED:
            emit("tap", p);
            break;
        case LV_EVENT_LONG_PRESSED:
            emit("long", p);
            break;
        case LV_EVENT_GESTURE: {
            lv_dir_t dir = lv_indev_get_gesture_dir(indev);
            emit(dir == LV_DIR_LEFT    ? "swipe_left"
                 : dir == LV_DIR_RIGHT ? "swipe_right"
                 : dir == LV_DIR_TOP   ? "swipe_up"
                                       : "swipe_down",
                 p);
            lv_indev_wait_release(indev);
            break;
        }
        default:
            break;
    }
}

static lv_obj_t* plain_obj(lv_obj_t* parent) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

void face_ui_create(void) {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(scr, screen_event_cb, LV_EVENT_ALL, NULL);

    for (int i = 0; i < 2; i++) {
        s_eyes[i] = plain_obj(scr);
        lv_obj_set_size(s_eyes[i], BOX_W, BOX_H);
        lv_obj_set_pos(s_eyes[i], LCD_H_RES / 2 + (i == 0 ? -EYE_DX : EYE_DX) - BOX_W / 2,
                       EYE_CY - BOX_H / 2);
        lv_obj_add_event_cb(s_eyes[i], draw_eye, LV_EVENT_DRAW_MAIN, NULL);
    }

    s_status = lv_label_create(scr);
    lv_obj_set_width(s_status, LCD_H_RES - 20);
    lv_obj_set_style_text_font(s_status, &font_noto_sans_basic_16_4, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(0x94A3B8), 0);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_MODE_DOTS);
    lv_label_set_text(s_status, "");
    lv_obj_align(s_status, LV_ALIGN_TOP_MID, 0, 4);

    s_text = lv_label_create(scr);
    lv_obj_set_size(s_text, LCD_H_RES - 16, 56);
    lv_obj_set_style_text_font(s_text, &font_noto_sans_basic_20_4, 0);
    lv_obj_set_style_text_color(s_text, lv_color_white(), 0);
    lv_obj_set_style_text_align(s_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(s_text, LV_LABEL_LONG_MODE_DOTS);
    lv_label_set_text(s_text, "");
    lv_obj_align(s_text, LV_ALIGN_BOTTOM_MID, 0, -4);

    // Calibration overlay on the top layer.
    s_calib_bg = plain_obj(lv_layer_top());
    lv_obj_set_size(s_calib_bg, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(s_calib_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_calib_bg, LV_OPA_COVER, 0);
    s_calib_h = plain_obj(s_calib_bg);
    lv_obj_set_size(s_calib_h, 31, 3);
    s_calib_v = plain_obj(s_calib_bg);
    lv_obj_set_size(s_calib_v, 3, 31);
    lv_obj_t* lines[] = {s_calib_h, s_calib_v};
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_bg_color(lines[i], lv_color_hex(0xF43F5E), 0);
        lv_obj_set_style_bg_opa(lines[i], LV_OPA_COVER, 0);
    }
    s_calib_label = lv_label_create(s_calib_bg);
    lv_obj_set_style_text_font(s_calib_label, &font_noto_sans_basic_20_4, 0);
    lv_obj_set_style_text_color(s_calib_label, lv_color_white(), 0);
    lv_obj_align(s_calib_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(s_calib_bg, LV_OBJ_FLAG_HIDDEN);

    s_next_blink = lv_tick_get() + 1500;
    s_next_glance = lv_tick_get() + 3000;
    face_ui_set_emotion("neutral");
    s_cur = s_target;
    lv_timer_create(tick_cb, TICK_MS, NULL);
}

void face_ui_set_event_cb(face_event_cb_t cb) { s_event_cb = cb; }

static bool has_word(const char* list, const char* word) {
    size_t n = strlen(word);
    for (const char* p = list; (p = strstr(p, word)) != NULL; p += n) {
        bool start = p == list || p[-1] == ' ';
        bool end = p[n] == '\0' || p[n] == ' ';
        if (start && end) {
            return true;
        }
    }
    return false;
}

void face_ui_set_emotion(const char* name) {
    const expression_t* ex = &kExpressions[0];
    for (size_t i = 0; i < sizeof(kExpressions) / sizeof(kExpressions[0]); i++) {
        if (name && has_word(kExpressions[i].names, name)) {
            ex = &kExpressions[i];
            break;
        }
    }
    s_target = ex->shape;
    s_color = lv_color_hex(ex->color);
    s_wink = ex->wink;
    s_expr_look_x = ex->look_x * 16;
    s_expr_look_y = ex->look_y * 12;
    s_look_tx = s_expr_look_x;
    s_look_ty = s_expr_look_y;
    s_look_hold_until = lv_tick_get() + 2000;
    lv_obj_invalidate(s_eyes[0]);
    lv_obj_invalidate(s_eyes[1]);
}

void face_ui_set_status(const char* text) { lv_label_set_text(s_status, text ? text : ""); }

void face_ui_set_text(const char* role, const char* text) {
    uint32_t color = 0xFFFFFF;
    if (role && strcmp(role, "user") == 0) {
        color = 0x94A3B8;
    } else if (role && strcmp(role, "system") == 0) {
        color = 0x67E8F9;
    }
    lv_obj_set_style_text_color(s_text, lv_color_hex(color), 0);
    lv_label_set_text(s_text, text ? text : "");
}

void face_ui_look(float x, float y) {
    x = x < -1 ? -1 : (x > 1 ? 1 : x);
    y = y < -1 ? -1 : (y > 1 ? 1 : y);
    s_look_tx = x * 18;
    s_look_ty = y * 12;
    s_look_hold_until = lv_tick_get() + 3000;
}

void face_ui_blink(void) {
    if (s_blink_start == 0) {
        s_blink_start = lv_tick_get();
    }
}

void face_ui_show_calibration_target(int x, int y, int step, int total) {
    lv_obj_set_pos(s_calib_h, x - 15, y - 1);
    lv_obj_set_pos(s_calib_v, x - 1, y - 15);
    lv_label_set_text_fmt(s_calib_label, "Tocá la cruz (%d/%d)", step, total);
    lv_obj_remove_flag(s_calib_bg, LV_OBJ_FLAG_HIDDEN);
}

void face_ui_hide_calibration(void) { lv_obj_add_flag(s_calib_bg, LV_OBJ_FLAG_HIDDEN); }
