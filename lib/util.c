#include "util.h"

void memcpy(void *source, void *destination, size_t n) {
    char *src = (char *)source;
    char *dest = (char *)destination;
    for(int i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}

void memset(void *ptr, int value, size_t n) {
    char *src = (char *)ptr;
    for (int i = 0; i < n; i++) {
        src[i] = value;
    }
}

void *memmove(void *source, void* destination, size_t n) {
    int *src = (int *)source;
    int *dest = (int *)destination;
    if(dest > src && dest < src + n) {
        size_t i = n;
        while(i-- > 0) {
            dest[i] = src[i];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            dest[i] = src[i];
        }
    }
    return destination;
}

int memcmp(const void *ptr1, const void *ptr2, size_t n) {
    unsigned char *a = (unsigned char *)ptr1;
    unsigned char *b = (unsigned char *)ptr2;
    for (size_t i = 0; i < n; i++) {
        if(a[i] != b[i]) {
            return (int)(a[i]-b[i]);
        }
    }
    return 0;
}

int atoi(char *str) {

    bool minus = false;
    int i = 0;
    if(str[0] == '-'){
        minus = true;
        i = 1;
    } else if(str[0] == '+') {
        i = 1;
    }
    int value = 0;
    while(str[i]) {
        if(str[i] >= '0' && str[i] <= '9') {
            value = value * 10 + (str[i] - '0');
        } else {
            break;
        }
        i++;
    }
    if(minus) {
        return -value;
    }
    return value;
}

void to_string(int value, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    if (value == 0) {
        buffer[i++] = '0';
    }

    // tulis digit ke buffer secara terbalik
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // balik string
    for (int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = tmp;
    }
}
