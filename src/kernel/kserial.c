#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <serial.h>
#include <kstring.h>

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

void kserial_outf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    uint64_t i;
    int64_t d;
    char *s;
    char c;
    int length;
    int len;
    int j;
    
    for (int count = 0; count < str_len(fmt); count++)
    {
        if (fmt[count] != '%')
        {
            kserial_outc(fmt[count]);
            continue;
        }

        count++;

        length = 0;
        len = 0;
        j = 0;

        if (fmt[count] >= '0' && fmt[count] <= '9')
        {
            while (fmt[count] >= '0' && fmt[count] <= '9')
            {
                length = 10 * length + fmt[count] - '0';
                count++;
            }
        }
        else if (fmt[count] == '*')
        {
            length = va_arg(arg, int);
            count++;
        }

        switch (fmt[count])
        {
            case 's':
                s = va_arg(arg, char *);
                if (length == 0)
                {
                    kserial_outs(s);
                    break;
                }
                len = str_len(s);
                j = 0;
                while (j < length)
                {
                    if (j > len)
                        kserial_outc(' ');
                    else
                        kserial_outc(s[j]);
                    j++;
                }
                break;
            case 'c':
                c = va_arg(arg, int);
                if (c == 0)
                    break;
                kserial_outc(c);
                break;
            case 'b':
                i = va_arg(arg, uint64_t);
                baditoa(i, baditoa_buffer, 2);
                if (length == 0)
                {
                    kserial_outs(baditoa_buffer);
                    break;
                }
                for (len = str_len(baditoa_buffer); length > len; length--)
                    kserial_outc('0');
                kserial_outs(baditoa_buffer);
                break;
            case 'd':
                d = va_arg(arg, int64_t);
                baditoa(d, baditoa_buffer, 10);
                if (length == 0)
                {
                    kserial_outs(baditoa_buffer);
                    break;
                }
                for (len = str_len(baditoa_buffer); length > len; length--)
                    kserial_outc('0');
                kserial_outs(baditoa_buffer);
                break;
            case 'x':
                i = va_arg(arg, uint64_t);
                baditoa(i, baditoa_buffer, 16);
                if (length == 0)
                {
                    kserial_outs(baditoa_buffer);
                    break;
                }
                for (len = str_len(baditoa_buffer); length > len; length--)
                    kserial_outc('0');
                kserial_outs(baditoa_buffer);
                break;
            default:
                kserial_outc(fmt[count]);
                break;
        }
    }

    va_end(arg);
}