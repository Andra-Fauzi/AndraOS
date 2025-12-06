#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#pragma once

void memcpy(void *dest, const void *src, size_t n);
void memset(void *ptr, int value, size_t n);
void *memmove(void *source, void* destination, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
int atoi(char *str);
void to_string(int value, char *buffer);
uint16_t read_u16(uint8_t low, uint8_t high);
void substr(char *haystack, char needle, int length, char result[][128]);
void str_to_upper(char *str, int length);
