#pragma once

// UART link to the robot core (CN1) and to the USB serial port, one JSON object per line.
// See docs/heimdall/face-protocol.md.
void link_init(void);

// Sends one JSON line on both links (thread-safe). `json` must not contain a newline.
void link_send(const char* json);
