#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <mm/mem.h>

extern uint32_t *kscreen_termbuffer;

int kimage_istga(unsigned char *file)
{
    if (file[0] != 0 || file[1] != 0 || file[3] != 0 || file[4] != 0
        || file[5] != 0 || file[6] != 0 || file[7] != 0 || file[8] != 0
        || file[9] != 0 || (file[16] != 24 && file[16] != 32))
        return -1;
    return 1;
}

void kimage_termblit(uint32_t *image_ptr, int x, int y)
{
    if (!image_ptr)
        return;

    int xoff = 0, xlen = image_ptr[0];
    int yoff = 0, ylen = image_ptr[1];

    if (x < 0)
    {
        xoff -= x;
        x = 0;
    }
    if (y < 0)
    {
        yoff -= y;
        y = 0;
    }

    if (x + xlen >= k_infotable.k_graphics->horizontal_res)
        xlen = k_infotable.k_graphics->horizontal_res - x;
    if (y + ylen >= k_infotable.k_graphics->vertical_res)
        ylen = k_infotable.k_graphics->vertical_res - y;

    uint32_t *image_colors = image_ptr + 2;

    for (int h = 0; h < ylen; h++)
    {
        for (int w = 0; w < xlen; w++)
        {
            uint32_t color = image_colors[(w+xoff) + (image_ptr[1] - (h+yoff) - 1) * image_ptr[0]];

            unsigned where = (w+x)*4 + (y+h)*k_infotable.k_graphics->ppsl*4;
            ((unsigned char*)kscreen_termbuffer)[where] = color & 0xFF;
            ((unsigned char*)kscreen_termbuffer)[where + 1] = (color >> 8) & 0xFF;
            ((unsigned char*)kscreen_termbuffer)[where + 2] = (color >> 16) & 0xFF;
        }
    }
}

uint32_t *kimage_getbuftga(unsigned char *ptr, int size, size_t *pages)
{
    uint32_t *data;
    int i, j, k, x, y, w = (ptr[13] << 8) + ptr[12], h = (ptr[15] << 8) + ptr[14], o = (ptr[11] << 8) + ptr[10];
    int m = ((ptr[1]? (ptr[7]>>3)*ptr[5] : 0) + 18);

    kdebug_outf("\nkimage: w%d h%d", w, h);
    if (w < 1 || h < 1)
        return NULL;

    *pages = ((w * h + 2) * sizeof(int) + 0x1000 - 1) / 0x1000;
    data = kmem_alloc(*pages);
    if (!data)
        return NULL;

    kdebug_outf("\nkimage: tga type [%d] {%x}", ptr[2], (uint64_t)ptr);
    switch(ptr[2]) {
        case 1:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { kmem_free(data, *pages); return NULL; }
            for(y=i=0; y<h; y++) {
                k = ((!o?h-y-1:y)*w);
                for(x=0; x<w; x++) {
                    j = ptr[m + k++]*(ptr[7]>>3) + 18;
                    data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                }
            }
            break;
        case 2:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { kmem_free(data, *pages); return NULL; }
            for(y=i=0; y<h; y++) {
                j = ((!o?h-y-1:y)*w*(ptr[16]>>3)) + 18;
                for(x=0; x<w; x++) {
                    data[2 + i++] = ((ptr[16]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    j += ptr[16]>>3;
                }
            }
            break;
        case 9:
            if(ptr[6]!=0 || ptr[4]!=0 || ptr[3]!=0 || (ptr[7]!=24 && ptr[7]!=32)) { kmem_free(data, *pages); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    j = ptr[m++]*(ptr[7]>>3) + 18;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                } else {
                    k++; x += k;
                    while(k--) {
                        j = ptr[m++]*(ptr[7]>>3) + 18;
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[7]==32?ptr[j+3]:0xFF) << 24) | (ptr[j+2] << 16) | (ptr[j+1] << 8) | ptr[j];
                    }
                }
            }
            break;
        case 10:
            if(ptr[5]!=0 || ptr[6]!=0 || ptr[1]!=0 || (ptr[16]!=24 && ptr[16]!=32)) { kmem_free(data, *pages); return NULL; }
            y = i = 0;
            for(x=0; x<w*h && m<size;) {
                k = ptr[m++];
                if(k > 127) {
                    k -= 127; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                    }
                    m += ptr[16]>>3;
                } else {
                    k++; x += k;
                    while(k--) {
                        if(!(i%w)) { i=((!o?h-y-1:y)*w); y++; }
                        data[2 + i++] = ((ptr[16]==32?ptr[m+3]:0xFF) << 24) | (ptr[m+2] << 16) | (ptr[m+1] << 8) | ptr[m];
                        m += ptr[16]>>3;
                    }
                }
            }
            break;
        default:
            kmem_free(data, *pages); return NULL;
    }
    data[0] = w;
    data[1] = h;

    return data;
}