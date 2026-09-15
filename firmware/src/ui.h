#pragma once

void ui_begin();
void ui_draw();
bool ui_handle_touch(int x, int y);
void ui_touch_release(int x, int y);
bool ui_setup_open();
void ui_setup_cancel();
bool ui_setup_save();
void ui_setup_toggle_kb();
