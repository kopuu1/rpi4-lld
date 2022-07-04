#include "mem.h"

void *memcpy(const void *src, void *dest, u32 n) {
    //simple implementation...
    u8 *bsrc = (u8 *)src;
    u8 *bdest = (u8 *)dest;

    for (int i=0; i<n; i++) {
        bdest[i] = bsrc[i];
    }

    return dest;
}
