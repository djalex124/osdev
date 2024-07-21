#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <serial.h>

void kserial_init()
{
    outb(PORT1 + 1, 0x00);
	outb(PORT1 + 3, 0x80);
	outb(PORT1 + 0, 0x03);
	outb(PORT1 + 1, 0x00);
	outb(PORT1 + 3, 0x03);
	outb(PORT1 + 2, 0xC7);
	outb(PORT1 + 4, 0x0B);
}

int kserial_empty()
{
    return inb(PORT1 + 5) & 0x20;
}

void kserial_outc(char c)
{
    while (!kserial_empty());
    outb(PORT1, c);
}

size_t str_len(const char* s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

void kserial_outs(char *s)
{
    for (size_t i = 0; i < str_len(s); i++)
        kserial_outc(s[i]);
}

static char baditoa_buffer[32];

void baditoa(uint64_t i, char buf[], int b)
{
    char* p = buf;
    if (i < 0 && b == 10)
    {
        *p++ = '-';
        i *= -1;
    }

    char* low = p;
    do
    {
        *p++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + i % b];
        i /= b;
    }while (i);

    *p-- = '\0';
    while (low < p)
    {
        char tmp = *low;
        *low++ = *p;
        *p-- = tmp;
    }
}

void kserial_outn(uint64_t n, uint8_t b)
{
    baditoa(n, baditoa_buffer, b);
    kserial_outs(baditoa_buffer);
}

void kserial_outf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    uint64_t i;
    char *s;
    
    for (int count = 0; count < str_len(fmt); count++)
    {
        if (fmt[count] != '%')
        {
            kserial_outc(fmt[count]);
            continue;
        }

        count++;

        switch (fmt[count])
        {
            case 's':
                s = va_arg(arg, char *);
                if (s == 0)
                    break;
                kserial_outs(s);
                break;
            case 'b':
                i = va_arg(arg, uint64_t);
                kserial_outn(i, 2);
                break;
            case 'd':
                i = va_arg(arg, uint64_t);
                kserial_outn(i, 10);
                break;
            case 'x':
                i = va_arg(arg, uint64_t);
                kserial_outn(i, 16);
                break;
            default:
                kserial_outc(fmt[count]);
                break;
        }
    }

    va_end(arg);
}