#include <stdint.h>
#include <stddef.h>

static inline uint8_t inb(uint16_t p)
{
    uint8_t ret;
    asm volatile (
        "inb %1, %0"
        : "=a"(ret)
        : "Nd"(p)
        : "memory");
    return ret;
}

static inline void outb(uint16_t p, uint8_t v)
{
    asm volatile ( "outb %0, %1" : : "a"(v), "Nd"(p) :"memory" );
}

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