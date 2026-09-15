#pragma once

#include <cstddef>

void pwr_begin();
void pwr_note_activity();
void pwr_poll();
void pwr_format_hub(char* out, size_t n);
int pwr_die_c();
