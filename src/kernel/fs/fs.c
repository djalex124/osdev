#include <kstring.h>
#include <stdint.h>
#include <kernel.h>
#include <screen.h>
#include <debug.h>
#include <mem.h>
#include <pci.h>
#include <fs.h>

extern kfs_patadrive kfs_patadrives[4];

void kfs_printinfo()
{
    kscreen_putf("\nkfs devices currently detected\npata devices:");
    for (int i = 0; i < 4; i++)
    {
        if (kfs_patadrives[i].exists)
        {
            uint64_t size = kfs_patadrives[i].sectors * kfs_patadrives[i].sector_size;
            
            int a = -1, b = 0;
            char* trim = kfs_patadrives[i].model;
            while (trim[b] != '\0')
            {
                if (trim[b] != ' ')
                    a = b;
                b++;
            }
            trim[a + 1] = '\0';

            kscreen_putf("\n - device %d [%s] %d MB", i, trim, size / 1024 / 1024);
        }
    }
}

void kfs_init()
{
    for (size_t i = 0; i < k_infotable.kpci_tablesize; i++)
    {
        if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x1)
            kfs_patainit(&k_infotable.kpci_table[i]);
        else if (k_infotable.kpci_table[i].class == 0x1 && k_infotable.kpci_table[i].subclass == 0x6)
            kfs_satainit(&k_infotable.kpci_table[i]);
    }
}