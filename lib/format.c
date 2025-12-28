#include "format.h"

static size_t kstrlen(const char *s)
{
    size_t len = 0;
    while (s && s[len]) len++;
    return len;
}

static void reverse(char *buf, size_t len)
{
    for (size_t i = 0; i < len / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[len - i - 1];
        buf[len - i - 1] = tmp;
    }
}

static size_t utoa(unsigned long val, char *buf, int base, int uppercase)
{
    const char *digits = uppercase ?
        "0123456789ABCDEF" :
        "0123456789abcdef";

    size_t i = 0;

    if (val == 0) {
        buf[i++] = '0';
        return i;
    }

    while (val) {
        buf[i++] = digits[val % base];
        val /= base;
    }

    reverse(buf, i);
    return i;
}

static size_t itoa(long val, char *buf)
{
    size_t i = 0;
    unsigned long u;

    if (val < 0) {
        buf[i++] = '-';
        u = (unsigned long)(-val);
    } else {
        u = (unsigned long)val;
    }

    i += utoa(u, buf + i, 10, 0);
    return i;
}

int vsnprintf(char *out, size_t size, const char *fmt, va_list args)
{
    size_t pos = 0;

    if (size == 0)
        return 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            if (pos + 1 < size)
                out[pos] = *fmt;
            pos++;
            continue;
        }

        fmt++; // skip %

        char tmp[32];
        size_t len = 0;

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(args, const char *);
            if (!s) s = "(null)";
            len = kstrlen(s);
            for (size_t i = 0; i < len; i++) {
                if (pos + 1 < size)
                    out[pos] = s[i];
                pos++;
            }
            break;
        }

        case 'c': {
            char c = (char)va_arg(args, int);
            if (pos + 1 < size)
                out[pos] = c;
            pos++;
            break;
        }

        case 'd':
        case 'i':
            len = itoa(va_arg(args, int), tmp);
            goto copy;

        case 'u':
            len = utoa(va_arg(args, unsigned int), tmp, 10, 0);
            goto copy;

        case 'x':
            len = utoa(va_arg(args, unsigned int), tmp, 16, 0);
            goto copy;

        case 'X':
            len = utoa(va_arg(args, unsigned int), tmp, 16, 1);
            goto copy;

        case '%':
            if (pos + 1 < size)
                out[pos] = '%';
            pos++;
            break;

        default:
            // unknown specifier, print it raw
            if (pos + 1 < size)
                out[pos] = *fmt;
            pos++;
            break;

        copy:
            for (size_t i = 0; i < len; i++) {
                if (pos + 1 < size)
                    out[pos] = tmp[i];
                pos++;
            }
            break;
        }
    }

    // null-terminate
    if (pos < size)
        out[pos] = '\0';
    else
        out[size - 1] = '\0';

    return (int)pos;
}

