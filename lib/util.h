#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#pragma once

void memcpy(void *source, void *destination, size_t n);
void memset(void *ptr, int value, size_t n);
void *memmove(void *source, void* destination, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
int atoi(char *str);
void to_string(int value, char *buffer);