#ifdef AQUA_DEBUG

#include <debug.h>
#include <stdint.h>
#include <serial.h>
#include <kstring.h>

typedef struct __attribute__((packed))
{
    uint32_t length;
    uint16_t version;
    uint8_t unit_type;
    uint8_t pointer_size;
    uint32_t abbrev_offset;
}debuginfo_header;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint8_t tag;
    uint8_t children;
    uint8_t data[];
}debugabbrev_entry;

extern uint64_t _debug_info[];
extern uint64_t _debug_abbrev;
extern uint64_t _debug_str;
extern uint64_t _debug_line_str;
extern uint64_t _debug_line_str_end;

#define ptr_right(p, d) (uint64_t*)((uint64_t)(p) + d)
#define ptr_char(p) (uint8_t)*p

uint64_t kdbg_uleb128(uint8_t* num, uint64_t* value)
{
    uint64_t shift = 0, count = 0, result = 0;
    uint8_t byte;
    while (1)
    {
        byte = num[count];
        count++;
        result |= ((byte & 0x7F) << shift);
        if (!(byte & 0x80))
            break;
        shift += 7;
    }

    *value = result;
    return count;
}

uint64_t kdbg_sleb128(uint8_t* num, int64_t* value)
{
    uint64_t shift = 0, count = 0, result = 0;
    uint8_t byte;
    while (1)
    {
        byte = num[count];
        count++;
        result |= ((byte & 0x7F) << shift);
        shift += 7;
        if (!(byte & 0x80))
            break;
    }

    if ((shift < 64) && (byte & 0x40))
        result |= -(1 << shift);

    *value = result;
    return count;
}
/*
const char* kdbg_tags[] =
{
    "null", "TAG_array_type", "TAG_class_type", "TAG_entry_point",
    "TAG_enumeration_type", "TAG_formal_parameter", "resv", "resv",
    "TAG_imported_declaration", "resv", "TAG_label", "TAG_lexical_block",
    "resv", "TAG_member", "resv", "TAG_pointer_type",
    "TAG_reference_type", "TAG_compile_unit", "TAG_string_type", "TAG_structure_type",
    "resv", "TAG_subroutine_type", "TAG_typedef", "TAG_union_type",
    "TAG_unspecified_parameters", "TAG_variant", "TAG_common_block", "TAG_common_inclusion",
    "TAG_inheritance", "TAG_inlined_subroutine", "TAG_module", "TAG_ptr_to_member_type",
    "TAG_set_type", "TAG_subrange_type", "TAG_with_stmt", "TAG_access_declaration",
    "TAG_base_type", "TAG_catch_block", "TAG_const_type", "TAG_constant",
    "TAG_enumerator", "TAG_file_type", "TAG_friend", "TAG_namelist",
    "TAG_namelist_item", "TAG_packed_type", "TAG_subprogram", "TAG_template_type_parameter",
    "TAG_template_value_parameter", "TAG_try_block", "TAG_variant_part", "TAG_variable",
    "TAG_volatile_type", "TAG_dwarf_procedure", "TAG_restrict_type", "TAG_interface_type",
    "TAG_namespace", "TAG_imported_module", "TAG_unspecified_type", "TAG_partial_unit",
    "TAG_imported_unit", "resv", "TAG_condition", "TAG_shared_type",
    "TAG_type_unit", "TAG_rvalue_reference_type", "TAG_template_alias", "TAG_coarray_type",
    "TAG_generic_subrange", "TAG_dynamic_type", "TAG_atomic_type", "TAG_call_site",
    "TAG_call_site_parameter", "TAG_skeleton_unit", "TAG_immutable_type",
};

const char* kdbg_forms[] =
{
    "null", "FORM_addr", "resv", "FORM_block2",
    "FORM_block4", "FORM_data2", "FORM_data4", "FORM_data8",
    "FORM_string", "FORM_block", "FORM_block1", "FORM_data1",
    "FORM_flag", "FORM_sdata", "FORM_strp", "FORM_udata",
    "FORM_ref_addr", "FORM_ref1", "FORM_ref2", "FORM_ref4",
    "FORM_ref8", "FORM_ref_udata", "FORM_indirect", "FORM_sec_offset",
    "FORM_exprloc", "FORM_flag_present", "FORM_strx", "FORM_addrx",
    "FORM_ref_sup4", "FORM_strp_sup", "FORM_data16", "FORM_line_strp",
    "FORM_ref_sig8", "FORM_implicit_const", "FORM_loclistx", "FORM_rnglistx",
    "FORM_ref_sup8", "FORM_strx1", "FORM_strx2", "FORM_strx3",
    "FORM_strx4", "FORM_addrx1", "FORM_addrx2", "FORM_addrx3",
    "FORM_addrx4"
};

const char* kdbg_ats[] =
{
    "null", "AT_sibling", "AT_location", "AT_name",
    "resv", "resv", "resv", "resv",
    "resv", "AT_ordering", "resv", "AT_byte_size",
    "resv", "AT_bit_size", "resv", "resv",
    "AT_stmt_list", "AT_low_pc", "AT_high_pc", "AT_language",
    "resv", "AT_discr", "AT_discr_value", "AT_visibility",
    "AT_import", "AT_string_length", "AT_common_reference", "AT_comp_dir",
    "AT_const_value", "AT_containing_type", "AT_default_value", "resv",
    "AT_inline", "AT_is_optional", "AT_lower_bound", "resv",
    "resv", "AT_producer", "resv", "AT_prototyped",
    "resv", "resv", "AT_return_addr", "resv",
    "AT_start_scope", "resv", "AT_bit_stride", "AT_upper_bound", "resv",
    "AT_abstract_origin", "AT_accessibility", "AT_address_class", "AT_artificial",
    "AT_base_types", "AT_calling_convention", "AT_count", "AT_data_member_location",
    "AT_decl_column", "AT_decl_file", "AT_decl_line", "AT_declaration",
    "AT_discr_list", "AT_encoding", "AT_external", "AT_frame_base",
    "AT_friend", "AT_identifier_case", "resv", "AT_namelist_item",
    "AT_priority", "AT_segment", "AT_specification", "AT_static_link",
    "AT_type", "AT_use_location", "AT_variable_parameter", "AT_virtuality",
    "AT_vtable_elem_location", "AT_allocated", "AT_associated", "AT_data_location",
    "AT_byte_stride", "AT_entry_pc", "AT_use_UTF8", "AT_extension", "AT_ranges",
    "AT_trampoline", "AT_call_column", "AT_call_file", "AT_call_line",
    "AT_description", "AT_binary_scale", "AT_decimal_scale", "AT_small",
    "AT_decimal_sign", "AT_digit_count", "AT_picture_string", "AT_mutable",
    "AT_threads_scaled", "AT_explicit", "AT_object_pointer", "AT_endianity",
    "AT_elemental", "AT_pure", "AT_recursive", "AT_signature",
    "AT_main_subprogram", "AT_data_bit_offset", "AT_const_expr", "AT_enum_class",
    "AT_linkage_name", "AT_string_length_bit_size", "AT_string_length_byte_size", "AT_rank",
    "AT_str_offsets_base", "AT_addr_base", "AT_rnglists_base", "resv",
    "AT_dwo_name", "AT_reference", "AT_rvalue_reference", "AT_macros",
    "AT_call_all_calls", "AT_call_all_source_calls", "AT_all_tail_calls", "AT_call_return_pc",
    "AT_call_value", "AT_call_origin", "AT_call_parameter", "AT_call_pc",
    "AT_call_tail_call", "AT_call_target", "AT_call_target_clobbered", "AT_call_data_location",
    "AT_call_data_value", "AT_noreturn", "AT_alignment", "AT_export_symbols",
    "AT_deleted", "AT_defaulted", "AT_loclists_base",
};
*/
static char kdbg_string[64];
static char kdbg_file[64];

void kdbg_trace(uint64_t addr)
{
    debuginfo_header* debug = (debuginfo_header *)(uint64_t)&_debug_info;
    uint64_t* ptr = ptr_right(debug, sizeof(debuginfo_header));
    uint64_t* abbrev_ptr = (uint64_t *)(uint64_t)debug->abbrev_offset;

    uint8_t level = 0, unit = 0;
    uint64_t size = 1;

    uint64_t line = 0, column = 0, high = 0, low = 0;

    while (1)
    {
        uint64_t length = (uint64_t)ptr + debug->length;
        
        uint8_t check = ptr_char(ptr);

        debugabbrev_entry* entry = (debugabbrev_entry*) abbrev_ptr;
        uint64_t index = 0, uleb_num;
        int64_t sleb_num = 0;
        uint64_t leb_size = 0;

        while ((uint64_t)ptr < length)
        {   
            entry = (debugabbrev_entry*) abbrev_ptr;
            size = 0;

            if(check==entry->type)
            {
                if (entry->children == 1)
                    level++;
            }

            for (index = 0; ; index+=2)
            {   
                if (check == 0)
                    break;

                if (entry->data[index] == 0xb7 || entry->data[index] == 0xb8 || entry->data[index] == 0x82)
                {
                    index--; // tag for gnu offsets and at_tail_call
                    continue;
                }

                uint8_t type = entry->data[index + 1];
                switch (type)
                {
                    case 0x1E: // DW_FORM_data16
                        size += 16;
                        break;
                    case 0x1: // DW_FORM_addr
                        if (entry->data[index] == 0x88)
                        {
                            index++;
                            size++;
                            break;
                        }
                        if((check == entry->type) && (entry->tag == 0x2e))
                        {
                            switch (entry->data[index])
                            {
                                case 0x11:
                                    low = *ptr_right(ptr, size + 1);
                            }
                        }
                        size += debug->pointer_size;
                        break;
                    case 0x7: // DW_FORM_data8
                    case 0x10:// DW_FORM_ref_addr
                    case 0x14:// DW_FORM_ref8
                    case 0x20:// DW_FORM_ref_sig8
                    case 0x24:// DW_FORM_ref_sup8
                        if((check == entry->type) && (entry->tag == 0x2e))
                        {
                            switch (entry->data[index])
                            {
                                case 0x12:
                                    high = *ptr_right(ptr, size + 1);
                            }
                        }
                        size += 8;
                        break;
                    case 0xE: // DW_FORM_strp
                        if(check == entry->type)
                        {
                            switch (entry->tag)
                            {
                                case 0x11:
                                    char* strp = (char*)(uintptr_t)((uint32_t)*ptr_right(ptr, size + 1));
                                    if (entry->data[index] == 0x3)
                                        memcpy(kdbg_file, strp, str_len(strp) + 1);
                                    break;
                                case 0x2e:
                                    strp = (char*)(uintptr_t)((uint32_t)*ptr_right(ptr, size + 1));
                                    memcpy(kdbg_string, strp, str_len(strp) + 1);
                                    break;
                            }
                        }
                        size += 4;
                        break;
                    case 0x1F:// DW_FORM_line_strp
                        if(check == entry->type)
                        {
                            switch (entry->tag)
                            {
                                case 0x11:
                                    char* strp = (char*)(uintptr_t)((uint32_t)*ptr_right(ptr, size + 1));
                                    if (entry->data[index] == 0x3)
                                        memcpy(kdbg_file, strp, str_len(strp) + 1);
                                    break;
                                case 0x2e:
                                    strp = (char*)(uintptr_t)((uint32_t)*ptr_right(ptr, size + 1));
                                    memcpy(kdbg_string, strp, str_len(strp) + 1);
                                    break;
                            }
                        }
                        size += 4;
                        break;
                    case 0x13:// DW_FORM_ref4
                    case 0x17:// DW_FORM_sec_offset (?)
                    case 0x1C:// DW_FORM_ref_sup4
                    case 0x1D:// DW_FORM_strp_sup
                    case 0x28:// DW_FORM_strx4
                    case 0x2C:// DW_FORM_addrx4
                        size += 4;
                        break;
                    case 0x4: // DW_FORM_block4
                    case 0x6: // DW_FORM_data4
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
                    case 0x25:// DW_FORM_strx1
                    case 0x29:// DW_FORM_addrx1
                        if((check == entry->type) && (entry->tag == 0x2e))
                        {
                            switch (entry->data[index])
                            {
                                case 0x3b:
                                    line = (uint8_t)*ptr_right(ptr, size + 1);
                                    break;
                                case 0x39:
                                    column = (uint8_t)*ptr_right(ptr, size + 1);
                                    break;
                            }
                        }
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
                        leb_size = 0;
                        leb_size = kdbg_uleb128((uint8_t *)((uint64_t)ptr + size + 1), &uleb_num);
                        size += leb_size;
                        break;
                    case 0xD: // DW_FORM_sdata
                    case 0x21:// DW_FORM_implicit_const
                        leb_size = 0;
                        index++;
                        leb_size = kdbg_sleb128(&entry->data[index + 1], &sleb_num);
                        if((check == entry->type) && (entry->tag == 0x2e))
                        {
                            switch (entry->data[index])
                            {
                                case 0x3b:
                                    line = (uint64_t)sleb_num;
                                    break;
                                case 0x39:
                                    column = (uint64_t)sleb_num;
                                    break;
                            }
                        }
                        break;
                    case 0x8: // DW_FORM_string
                        char *str = (char *)((uint64_t)ptr + size + 1);
                        uint64_t str_size = str_len(str) + 1;
                        size += str_size;
                        if(check == entry->type)
                        {
                            switch (entry->tag)
                            {
                                case 0x11:
                                    memcpy(kdbg_file, str, str_size);
                                    break;
                                case 0x2e:
                                    memcpy(kdbg_string, str, str_size);
                                    break;
                            }
                        }
                        break;
                    case 0x18:// DW_FORM_exprloc
                        leb_size = kdbg_uleb128((uint8_t *)((uint64_t)ptr + size + 1), &uleb_num);
                        size += uleb_num + 1;
                        break;
                    case 0x19:// DW_FORM_flag_present
                    case 0:
                        break;
                }
                if ((entry->data[index] == 0) && (entry->data[index + 1] == 0))
                    break;
            }
            abbrev_ptr = ptr_right(abbrev_ptr, index + 5);
            if (check==entry->type)
                break;
        }

        if (entry->tag == 0x2e)
        {
            if ((addr >= low) && (addr <= low + high))
            {
                kserial_outf("\r\nkdbg: (%x) [%s] line:%d column:%d function:%s", addr, kdbg_file, line, column, kdbg_string);
                break;
            }
            high = 0;
            low = 0;
            kdbg_string[0] = 0;
        }

        if(check == 0)level--;
        abbrev_ptr = (uint64_t *)(uint64_t)debug->abbrev_offset;
        ptr = ptr_right(ptr, size + 1);
        if (level == 0)
        {
            debug = (debuginfo_header *)ptr;
            ptr = ptr_right(ptr, sizeof(debuginfo_header));
            abbrev_ptr = (uint64_t *)(uint64_t)debug->abbrev_offset;
            size = 1;
            unit++;
            kdbg_file[0] = 0;
        }
        if ((uint64_t)ptr > (uint64_t)&_debug_abbrev)
            break;
    }
}

#endif