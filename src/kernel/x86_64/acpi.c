#include <acpi.h>
#include <debug.h>
#include <mem.h>
#include <kstring.h>

extern boot_table ktable;

void kacpi_init()
{
    kdebug_outf("\r\nkacpi: rsdp at 0x%x", ktable.rsdp);
    
    kmem_page(ktable.rsdp & 0xFFFFF000, ktable.rsdp & 0xFFFFF000, 0x1000, 0b11);
    struct acpi_rsdp *table = (struct acpi_rsdp *)ktable.rsdp;

    kdebug_outf("\r\nkacpi: revision %8s", (char *)table->signature);
}