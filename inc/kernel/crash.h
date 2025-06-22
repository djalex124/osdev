#pragma once

#ifdef AQUA_DEBUG
void kcrash_initsym();
#endif

void kcrash(char *message);