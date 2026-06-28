#pragma once

#include <x86_64/acpi/acpi.h>

typedef struct
{
    acpi_madt_header h;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
}acpi_madt_type0;

typedef struct
{
    acpi_madt_header h;
    uint8_t io_apic_id;
    uint8_t zero;
    uint32_t io_apic_addr;
    uint32_t gsi_base;
}acpi_madt_type1;

typedef struct
{
    acpi_madt_header h;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
}acpi_madt_type2;

typedef struct
{
    acpi_madt_header h;
    uint8_t nmi_source;
    uint8_t zero;
    uint16_t flags;
    uint32_t gsi;
}acpi_madt_type3;

typedef struct
{
    acpi_madt_header h;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
}__attribute__((packed)) acpi_madt_type4;

typedef struct
{
    acpi_madt_header h;
    uint16_t zero;
    uint64_t local_apic_addr;
}acpi_madt_type5;

typedef struct
{
    acpi_madt_header h;
    uint16_t zero;
    uint64_t processor_x2apic_id;
    uint64_t flags;
    uint64_t acpi_id;
}acpi_madt_type9;

union ioredtbl_entry
{
    struct
    {
        uint64_t int_vector:8;
        uint64_t deliv_mode:3;
        uint64_t desti_mode:1;
        uint64_t deliv_stat:1;
        uint64_t pin_polarity:1;
        uint64_t remote_irr:1;
        uint64_t trigger_mode:1;
        uint64_t mask:1;
        uint64_t reserved:39;
        uint64_t destination:8;
    };
    struct
    {
        uint32_t lower;
        uint32_t upper;
    };
};

void kacpi_processapic(acpi_madt *madt);

#define APIC_REG_EOI   0x0B0
#define APIC_REG_SIV   0x0F0
#define APIC_REG_ERROR 0x280
#define APIC_REG_ICR_1 0x300
#define APIC_REG_ICR_2 0x310