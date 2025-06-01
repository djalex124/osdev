#pragma once

#include <stdint.h>

#ifdef AQUA_DEBUG

#include <output/serial.h>

#define kdebug_outf(format, ...) kserial_outf(format __VA_OPT__(,) __VA_ARGS__)

#else

#define kdebug_outf(format, ...)

#endif