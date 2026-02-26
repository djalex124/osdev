#include <stdatomic.h>

#include <mm/mem.h>

atomic_flag *ksync_mutex_new()
{
    return kmem_kalloc(sizeof(atomic_flag));
}

void ksync_mutex_acq(atomic_flag *mutex)
{
    while (atomic_flag_test_and_set(mutex));
}

void ksync_mutex_rel(atomic_flag *mutex)
{
    atomic_flag_clear(mutex);
}

void ksync_mutex_destroy(atomic_flag *mutex)
{
    kmem_kfree(mutex);
}