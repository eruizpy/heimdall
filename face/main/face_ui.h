#pragma once

// Robot face: animated eyes, status line and chat text. All functions need the LVGL lock
// (lvgl_port_lock) except face_ui_set_event_cb.

// Called on touch gestures: ev is "tap", "long", "swipe_left", "swipe_right", "swipe_up",
// "swipe_down".
typedef void (*face_event_cb_t)(const char* ev, int x, int y);

void face_ui_create(void);
void face_ui_set_event_cb(face_event_cb_t cb);

void face_ui_set_emotion(const char* name);
void face_ui_set_status(const char* text);
void face_ui_set_text(const char* role, const char* text);
void face_ui_look(float x, float y);  // -1..1
void face_ui_blink(void);

void face_ui_show_calibration_target(int x, int y, int step, int total);
void face_ui_hide_calibration(void);
