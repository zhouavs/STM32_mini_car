#pragma once

#include <stdlib.h>

#define ASCII_CHAR_COUNT 128
#define ASCII_CHAR_WIDTH 8
#define ASCII_CHAR_HEIGHT 16

typedef uint8_t ascii_font_t[16];

const ascii_font_t *Font_get_ascii(const uint8_t c);
