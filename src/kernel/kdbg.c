#include <debug.h>
#include <stdint.h>
#include <serial.h>
#include <kstring.h>

typedef struct __attribute__((packed))
{
    uint32_t length;
    uint8_t version;
    uint8_t abbrev_offset;
    uint8_t unit_type;
    uint8_t pointer_size;
}debuginfo_header;

typedef struct __attribute__((packed))
{
    uint32_t producer;
    uint8_t language;
    uint32_t name_string;
    uint32_t dir_string;
    uint64_t low_pc;
    uint64_t high_pc;
    uint32_t stmt_list;
}dw_tag_compile_unit;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint8_t tag;
    uint8_t children;
    uint16_t data[];
}debugabbrev_entry;

extern char _debug_info;
extern char _debug_abbrev;

int is_valid_2char_string(char* str)
{
    if (str_len(str) != 2)
        return 0;
    else if (!(str[0]>='0'&&str[0]<='9') && !(str[0]>='A'&&str[0]<='Z') && !(str[0]>='a'&&str[0]<='z'))
        return 0;
    else if (!(str[1]>='0'&&str[1]<='9') && !(str[1]>='A'&&str[1]<='Z') && !(str[1]>='a'&&str[1]<='z'))
        return 0;
    return 1;
}

#define ptr_right(p, d) (uint64_t*)((uint64_t)(p) + d)
#define ptr_char(p) (uint8_t)*p

uint64_t kdbg_decode_uleb128(const uint8_t *p, uint64_t *n)
{
    uint64_t num = 0;
    int shift = 0;
    int next = 0;

    while (1)
    {
        uint8_t byte = p[next];
        next++;
        num |= (byte & 0x7F) << shift;
        if ((byte >> 7) == 0)
            break;
        shift += 7;
    }

    *n = num;
    return next;
}

int64_t kdbg_decode_sleb128(const uint8_t *p, int64_t *n)
{
    int64_t num = 0;
    int shift = 0;
    int next = 0;

    uint8_t byte = p[next];

    while ((byte >> 7) != 0)
    {
        byte = p[next];
        num |= (byte & 0x7F) << shift;
        shift += 7;
        next++;
    }

    if ((shift < next) && ((byte >> 6) & 0x1))
        num |= (~0 << shift);

    *n = num;
    return next;
}

char* kdbg_trace(uint64_t addr)
{
    char* temp = "\0";

    debuginfo_header* debug = (debuginfo_header *)&_debug_info;
    uint64_t* ptr = ptr_right(debug, sizeof(debuginfo_header));
    ptr = ptr_right(ptr, 4);

    while ((uint64_t)ptr < (uint64_t)&_debug_abbrev)
    {
        uint8_t check = ptr_char(ptr);

        kserial_outf("\r\nkdbg: first number > %x", check);

        uint64_t* abbrev_ptr = ptr_right(&_debug_abbrev, debug->abbrev_offset);
        debugabbrev_entry* entry;
        uint64_t size, index;

        while (check != entry->type)
        {
            entry = (debugabbrev_entry*) abbrev_ptr;
            size = 0;
            
            kserial_outf("\r\nkdbg: abbrev > %x", entry->type);
            kserial_outf("\r\nkdbg:     flag > %x", entry->tag);

            for (index = 0; entry->data[index] != 0; index++)
            {
                kserial_outf("\r\nkdbg:         index > %x", index);
                kserial_outf("\r\nkdbg:         data > %x", entry->data[index]);

                uint8_t type = entry->data[index] >> 8;
                switch (type)
                {
                    case 0x1E: // DW_FORM_data16
                        size += 16;
                        break;
                    case 0x1: // DW_FORM_addr
                    case 0x7: // DW_FORM_data8
                    case 0x10:// DW_FORM_ref_addr
                    case 0x14:// DW_FORM_ref8
                    case 0x20:// DW_FORM_ref_sig8
                    case 0x24:// DW_FORM_ref_sup8
                        size += 8;
                        break;
                    case 0x4: // DW_FORM_block4
                    case 0x6: // DW_FORM_data4
                    case 0xE: // DW_FORM_strp
                    case 0x13:// DW_FORM_ref4
                    case 0x17:// DW_FORM_sec_offset (?)
                    case 0x1C:// DW_FORM_ref_sup4
                    case 0x1D:// DW_FORM_strp_sup
                    case 0x1F:// DW_FORM_line_strp
                    case 0x28:// DW_FORM_strx4
                    case 0x2C:// DW_FORM_addrx4
                        size += 4;
                        break;
                    case 0x27:// DW_FORM_strx3
                    case 0x2B:// DW_FORM_addrx3
                        size += 3;
                        break;
                    case 0x3: // DW_FORM_block2
                    case 0x5: // DW_FORM_data2
                    case 0x12:// DW_FORM_ref2
                    case 0x26:// DW_FORM_strx2
                    case 0x2A:// DW_FORM_addrx2
                        size += 2;
                        break;
                    case 0xA: // DW_FORM_block1
                    case 0xB: // DW_FORM_data1
                    case 0xC: // DW_FORM_flag
                    case 0x11:// DW_FORM_ref1
                    case 0x19:// DW_FORM_flag_present
                    case 0x25:// DW_FORM_strx1
                    case 0x29:// DW_FORM_addrx1
                        size += 1;
                        break;
                    case 0x9: // DW_FORM_block
                    case 0xF: // DW_FORM_udata
                    case 0x15:// DW_FORM_ref_udata
                    case 0x16:// DW_FORM_indirect
                    case 0x1A:// DW_FORM_strx
                    case 0x1B:// DW_FORM_addrx
                    case 0x22:// DW_FORM_loclistx
                    case 0x23:// DW_FORM_rnglistx
                        uint64_t unum = 0;
                        uint8_t *uleb_start = (uint8_t *)((uint64_t)ptr + size);
                        kserial_outf("\r\nkdbg:             uleb > %x", ((uint64_t)ptr + size));
                        size += kdbg_decode_uleb128(uleb_start, &unum);
                        break;
                    case 0xD: // DW_FORM_sdata
                    case 0x21:// DW_FORM_implicit_const
                        int64_t snum = 0;
                        uint8_t *sleb_start = (uint8_t *)((uint64_t)ptr + size);
                        kserial_outf("\r\nkdbg:             sleb > %x", ((uint64_t)ptr + size));
                        size += kdbg_decode_sleb128(sleb_start, &snum);
                        break;
                    case 0x8: // DW_FORM_string
                        char *str = (char *)((uint64_t)ptr + size);
                        kserial_outf("\r\nkdbg:             str > %x size > %d", (uint64_t)sleb_start, str_len(str));
                        size += str_len(str);
                        break;
                    case 0x18:// DW_FORM_exprloc
                        //special case
                        size += -1;
                        break;
                }
            }
            kserial_outf("\r\nkdbg:     type > 0x%x size > 0x%x", entry->data[index] >> 8, size);

            abbrev_ptr = ptr_right(abbrev_ptr, index * 2 + 5);
        }

        //asm("hlt");
        ptr = ptr_right(ptr, size + 1);
    }

    return temp;
}