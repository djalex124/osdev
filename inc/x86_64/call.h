#pragma once

#include <stdint.h>

#define DECLARE_SYSCALL0(name)     int syscall_##name()
#define DECLARE_SYSCALL1(name, a1) int syscall_##name(a1)

#define DEFINE_SYSCALL0(name, call_num) \
    int syscall_##name() { \
        int ret; asm ("int $0x40" : "=a"(ret) : "a"(call_num)); \
        return ret; \
    }

#define DEFINE_SYSCALL1(name, call_num, A1) \
    int syscall_##name(A1 a1) { \
        int ret; asm ("int $0x40" : "=a"(ret) : "a"(call_num), "D"((uint64_t)a1)); \
        return ret; \
    }

DECLARE_SYSCALL0(test_print);
DECLARE_SYSCALL1(add_one, uint64_t);

void kcall_init();