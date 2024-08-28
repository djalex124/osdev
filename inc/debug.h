#pragma once

#include <stdint.h>

#ifdef AQUA_DEBUG

#define kdebug_outf(format, ...) kserial_outf(format __VA_OPT__(,) __VA_ARGS__)
void kdbg_trace(uint64_t addr);

#else

#define kdebug_outf(format, ...)

#endif