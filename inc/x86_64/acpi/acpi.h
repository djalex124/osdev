#pragma once

#include <stdint.h>

void kacpi_init();

typedef struct
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    uint32_t length; //only use these if revision 2.0+
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
}__attribute__((packed)) acpi_rsdp;

typedef struct
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
}acpi_sdt_header;

typedef struct
{
    acpi_sdt_header h;
    uint64_t other_sdt[];
}__attribute__((packed)) acpi_xsdt;

typedef struct
{
    acpi_sdt_header h;
    uint32_t other_sdt[];
}__attribute__((packed)) acpi_rsdt;

typedef struct
{
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
}acpi_gas;

typedef struct
{
    acpi_sdt_header h;
    uint32_t firmware_control;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t p_state_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t c_state_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    uint16_t boot_arch_flags; // acpi 2.0+

    uint8_t reserved_2;
    uint32_t Flags;

    acpi_gas reset_reg;

    uint8_t reset_value;
    uint8_t reserved_3[3];
  
    uint64_t x_firmware_control; // acpi 2.0+ from this point
    uint64_t x_dsdt;

    acpi_gas x_pm1a_event_block;
    acpi_gas x_pm1b_event_block;
    acpi_gas x_pm1a_control_block;
    acpi_gas x_pm1b_control_block;
    acpi_gas x_pm2_control_block;
    acpi_gas x_pm_timer_block;
    acpi_gas x_gpe0_block;
    acpi_gas x_gpe1_block;
}acpi_fadt;

typedef struct
{
    acpi_sdt_header h;
    uint8_t aml[];
}acpi_dsdt;

typedef struct
{
    uint64_t ecm_baseaddr;
    uint16_t pci_grpsegnum;
    uint8_t  pci_busnum;
    uint8_t  pci_busnumend;
    uint32_t reserved;
}acpi_mcfg_baa_header;

typedef struct
{
    acpi_sdt_header h;
    uint64_t reserved;
    acpi_mcfg_baa_header pci_baa[];
}acpi_mcfg;

typedef struct
{
    uint8_t entry_type;
    uint8_t entry_length;
}acpi_madt_header;

typedef struct
{
    acpi_sdt_header h;
    uint32_t local_apic_addr;
    uint32_t flags;
    uint8_t enteries[];
}acpi_madt;

void kacpi_processapic(acpi_madt *madt);
void kacpi_processdsdt(acpi_dsdt *dsdt);

void kacpi_shutdown();