#pragma once

#include "utils/types.h"
#include <stdio.h>

bool is_text(uint8 const* ptr, size_t size, double tolerance = 0.05f);
bool is_wave(uint8 const* ptr, size_t size);
bool is_mp3(uint8 const* ptr, size_t size, int check = 8);
bool is_mdx(uint8 const* ptr, size_t size);
