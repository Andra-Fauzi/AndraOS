#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

int vsnprintf(char *out, size_t size, const char *fmt, va_list args);
