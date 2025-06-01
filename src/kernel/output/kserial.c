#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/kstring.h>

#include <x86_64/port.h>

#define PORT1 0x3F8

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

void kserial_outf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);

    uint64_t unsign;
    int64_t sign;
    char *s;
    char c;
    int length;
    int len;
    int j;

    size_t strlen = str_len(fmt);
    
    for (int count = 0; count < strlen; count++)
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
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 2);
                if (length == 0)
                {
                    kserial_outs(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kserial_outc('0');
                kserial_outs(s);
                break;
            case 'd':
                sign = va_arg(arg, int64_t);
                s = str_itoa(sign, 10);
                if (length == 0)
                {
                    kserial_outs(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kserial_outc('0');
                kserial_outs(s);
                break;
            case 'x':
                unsign = va_arg(arg, uint64_t);
                s = str_utoa(unsign, 16);
                if (length == 0)
                {
                    kserial_outs(s);
                    break;
                }
                for (len = str_len(s); length > len; length--)
                    kserial_outc('0');
                kserial_outs(s);
                break;
            default:
                kserial_outc(fmt[count]);
                break;
        }
    }

    va_end(arg);
}