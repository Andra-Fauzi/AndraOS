#include "util.h"

void memcpy(void *dest, const void *src, size_t n) {
    const unsigned char *s = (const unsigned char *)src;
    unsigned char *d = (unsigned char *)dest;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
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

void substr(char *haystack, char needle, int length, char result[][128]) {
	int col = 0;
	int idx = 0;
	for (int i = 0; i < length; i++) {
		if(haystack[i] == needle) {
			result[col][idx] = '\0';
			idx = 0;
			col++;
		} else {
			result[col][idx] = haystack[i];
			idx++;
		}
	}
}

uint16_t read_u16(uint8_t low, uint8_t high) {
	return ((uint16_t)high << 8) | low;
}

void str_to_upper(char *str, int length) {
	for (int i = 0; i < length; i++) {
		if (str[i] >= 'a' && str[i] <= 'z') {
			str[i] = str[i] - 32;  // Convert to uppercase
		}
	}
}

void *calloc(size_t nmemb, size_t size) {
    // Cek overflow sederhana
    if (nmemb != 0 && size != 0) {
        size_t total = nmemb * size;
        if (total / size != nmemb) {
            // overflow jika jumlah terlalu besar
            return NULL;
        }

        void *ptr = malloc(total);
        if (!ptr) {
            return NULL;
        }

        // Isi semua dengan nol
        memset(ptr, 0, total);
        return ptr;
    }

    // Kalau jumlah atau size 0, bisa kembalikan NULL atau alokasi 0
    return NULL;
}

// strndup versi kamu
char *strndup(const char *s, size_t n) {
    if (!s)
        return NULL;

    // cari panjang asal sampai n
    size_t len = 0;
    while (len < n && s[len] != '\0') {
        len++;
    }

    // alokasi baru (len+1 untuk null)
    char *dup = (char *)malloc(len + 1);
    if (!dup)
        return NULL;

    // salin isi dan tambahkan null terminator
    memcpy(dup, s, len);
    dup[len] = '\0';

    return dup;
}

// Membandingkan dua string sampai n karakter
int strncmp(const char *s1, const char *s2, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];

        // jika karakter berbeda, kembalikan perbedaan (positif/negatif)
        if (c1 != c2)
            return (c1 < c2) ? -1 : 1;

        // jika salah satu sudah mencapai null terminator
        if (c1 == '\0')
            return 0;
    }
    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 != *s2) {
            // return selisih ASCII (atau hanya -1 / +1 jika mau)
            return (unsigned char)*s1 - (unsigned char)*s2;
        }
        s1++;
        s2++;
    }
    // jika salah satu atau kedua string sudah habis
    return (unsigned char)*s1 - (unsigned char)*s2;
}

size_t strlen(const char *s) {
    const char *p = s;
    // Loop sampai kita menemukan '\0'
    while (*p) {
        p++;
    }
    // Panjang = jumlah karakter sebelum null
    return (size_t)(p - s);
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *ret = dest;
    size_t i;

    // Salin karakter satu per satu sampai n atau sampai src habis
    for (i = 0; i < n && *src; i++) {
        *dest++ = *src++;
    }

    // Jika src habis sebelum n, pad dest sampai panjang n
    for (; i < n; i++) {
        *dest++ = '\0';
    }

    return ret;
}

char *strdup(const char *s) {
    // Hitung panjang string termasuk '\0'
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    // Tambah 1 untuk null terminator
    len++;

    // Alokasikan buffer panjang len
    char *copy = (char *)malloc(len);
    if (!copy) {
        // Gagal alokasi
        return NULL;
    }

    // Salin karakter satu per satu (termasuk '\0')
    for (size_t i = 0; i < len; i++) {
        copy[i] = s[i];
    }

    return copy;
}

// Alokasi ulang minimal
static char *realloc_buffer(char *buf, size_t old_cap, size_t new_cap) {
    char *new_buf = malloc(new_cap);
    if (!new_buf) return NULL;
    if (buf) {
        memcpy(new_buf, buf, old_cap);
        free(buf);
    }
    return new_buf;
}


// “open_memstream” versi mandiri
MemStream *memstream_open(void) {
    MemStream *ms = malloc(sizeof(MemStream));
    if (!ms) return NULL;
    ms->buf = NULL;
    ms->len = 0;
    ms->cap = 0;
    ms->pos = 0;
    return ms;
}

void memstream_write(MemStream *ms, const char *data, size_t size) {
    size_t needed = ms->pos + size;
    if (needed > ms->cap) {
        size_t new_cap = ms->cap ? ms->cap * 2 : 64;
        while (new_cap < needed) new_cap *= 2;
        ms->buf = realloc_buffer(ms->buf, ms->cap, new_cap);
        ms->cap = new_cap;
    }
    memcpy(ms->buf + ms->pos, data, size);
    ms->pos += size;
    if (ms->pos > ms->len) ms->len = ms->pos;
}

void memstream_seek(MemStream *ms, size_t new_pos) {
    if (new_pos <= ms->len) {
        ms->pos = new_pos;
    }
}

void memstream_flush(MemStream *ms, char **out_buf, size_t *out_len) {
    if (!ms) return;
    char *buf_terminated = malloc(ms->len + 1);
    memcpy(buf_terminated, ms->buf, ms->len);
    buf_terminated[ms->len] = '\0';
    *out_buf = buf_terminated;
    *out_len = ms->len;
}

void memstream_close(MemStream *ms) {
    if (!ms) return;
    free(ms->buf);
    free(ms);
}

void assert(int condition) {
    if (!condition) {
        // Disable interrupts
        __asm__ __volatile__("cli");
        
        // Halt the CPU
        while (1) {
            __asm__ __volatile__("hlt");
        }
    }
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    // Copy the data from the old block to the new block
    memcpy(new_ptr, ptr, size);
    free(ptr);
    return new_ptr;
}

// Character classification functions
int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

// String functions
char *strstr(const char *haystack, const char *needle) {
    if (!needle || !*needle) return (char *)haystack;
    
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

// Global stderr stream for chibicc error output
static char stderr_buffer[4096];
static MemStream stderr_stream_obj = {
    .buf = stderr_buffer,
    .len = 0,
    .cap = sizeof(stderr_buffer),
    .pos = 0
};
MemStream *stderr = &stderr_stream_obj;