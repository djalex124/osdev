#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <output/screen.h>

#include <mm/mem.h>

extern graphics_info kgraphics;
extern uint32_t *kscreen_termbuffer;

void kimage_termblit(uint32_t *image_ptr, int x, int y)
{
    if (!image_ptr)
        return;

    if (x + image_ptr[0] >= kgraphics.horizontal_res ||
        x < 0)
        return;
    else if (y + image_ptr[1] >= kgraphics.vertical_res ||
            y < 0)
        return;

    uint32_t *image_colors = image_ptr + 2;

    for (int h = 0; h < image_ptr[1]; h++)
    {
        for (int w = 0; w < image_ptr[0]; w++)
        {
            uint32_t color = image_colors[w + (image_ptr[1] - h - 1) * image_ptr[0]];

            unsigned where = (w+x)*4 + (y+h)*kgraphics.ppsl*4;
            ((unsigned char*)kscreen_termbuffer)[where] = color & 0xFF;
            ((unsigned char*)kscreen_termbuffer)[where + 1] = (color >> 8) & 0xFF;
            ((unsigned char*)kscreen_termbuffer)[where + 2] = (color >> 16) & 0xFF;
        }
    }
}

uint32_t *kimage_getbuftga(unsigned char *ptr, int size, size_t *pages)
{
    uint32_t *data;
    int i, k, x, y, w = (ptr[13] << 8) + ptr[12], h = (ptr[15] << 8) + ptr[14], o = (ptr[11] << 8) + ptr[10];
    int m = ((ptr[1]? (ptr[7]>>3)*ptr[5] : 0) + 18);

    kdebug_outf("\nkimage: w%d h%d", w, h);
    if (w < 1 || h < 1)
        return NULL;

    *pages = ((w * h + 2) * sizeof(int) + 0x1000 - 1) / 0x1000;
    data = kmem_alloc(*pages);
    if (!data)
        return NULL;

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
    data[0] = w;
    data[1] = h;

    return data;
}