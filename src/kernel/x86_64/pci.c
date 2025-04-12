#include <debug.h>
#include <screen.h>

void kpci_init()
{
    kdebug_outf("\r\nkpci_i: start iterate pci devices");
}

void kpci_printinfo()
{
    kscreen_putf("\nkpci_info: currently no devices :(");
}