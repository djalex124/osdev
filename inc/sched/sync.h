#pragma once

#include <stdatomic.h>

atomic_flag *ksync_mutex_new();

void ksync_mutex_acq(atomic_flag *mutex);
void ksync_mutex_rel(atomic_flag *mutex);
void ksync_mutex_destroy(atomic_flag *mutex);