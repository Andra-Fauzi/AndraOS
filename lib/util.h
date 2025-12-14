#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "memory.h"

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    size_t pos;
} MemStream;

void memcpy(void *dest, const void *src, size_t n);
void memset(void *ptr, int value, size_t n);
void *memmove(void *source, void* destination, size_t n);
int memcmp(const void *ptr1, const void *ptr2, size_t n);
int atoi(char *str);
void to_string(int value, char *buffer);
uint16_t read_u16(uint8_t low, uint8_t high);
void substr(char *haystack, char needle, int length, char result[][128]);
void str_to_upper(char *str, int length);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
char *strndup(const char *s, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
MemStream *memstream_open(void);
void memstream_write(MemStream *ms, const char *data, size_t size);
void memstream_seek(MemStream *ms, size_t new_pos);
void memstream_flush(MemStream *ms, char **out_buf, size_t *out_len);
void memstream_close(MemStream *ms);
char *strncpy(char *dest, const char *src, size_t n);
void assert(int condition);
int isspace(int c);
int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
char *strstr(const char *haystack, const char *needle);
int ispunct(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);
int isupper(int c);
char *strchr(const char *str, int c);
int strncasecmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
unsigned long strtoul(const char *str, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
long double strtold(const char *str, char **endptr);
char *strdup(const char *s);unsigned long strtoul(const char *str, char **endptr, int base);
long double strtold(const char *str, char **endptr);
void exit(int status);
