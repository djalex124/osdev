#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <output/screen.h>
#include <output/kterm.h>

#include <x86_64/desc.h>

#include <stdint.h>
#include </usr/include/elf.h>

extern int cw, ch;
uint8_t screen = 0;

#ifdef AQUA_DEBUG

extern uint64_t _end[];
uint64_t debug_addr = 0;

uint64_t debug_info = 0;
uint64_t debug_info_len = 0;
uint64_t debug_abbrev = 0;
uint64_t debug_str = 0;
uint64_t debug_line_str = 0;

void kcrash_initsym()
{
    debug_addr = ((uint64_t)_end / 0x1000) * 0x1000;

    Elf64_Ehdr *debug_ehdr = (Elf64_Ehdr *)debug_addr;
    if (debug_ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        debug_ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        debug_ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        debug_ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        debug_ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        debug_ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        debug_ehdr->e_type != ET_EXEC ||
        debug_ehdr->e_machine != EM_X86_64 ||
        debug_ehdr->e_version != EV_CURRENT)
    {
        kdebug_outf("\nkcrash: unable to read debug information!");
        return;
    }

    Elf64_Shdr *debug_shdrs = (Elf64_Shdr *)(debug_addr + debug_ehdr->e_shoff);
    uint64_t debug_shdrend = debug_ehdr->e_shnum * debug_ehdr->e_shentsize;

    Elf64_Shdr *shstrtab = (Elf64_Shdr *)((debug_ehdr->e_shstrndx * debug_ehdr->e_shentsize) + debug_ehdr->e_shoff + debug_addr);
    char *string = 0;

    for (Elf64_Shdr* debug_shdr = debug_shdrs;
        (char*)debug_shdr < (char*)debug_shdrs + debug_shdrend;
        debug_shdr = (Elf64_Shdr*)((char*)debug_shdr + debug_ehdr->e_shentsize))
    {
        string = (char *)(shstrtab->sh_offset + debug_shdr->sh_name + debug_addr);
        if (strn_cmp(string, ".debug_info", 12) == 0)
        {    
            debug_info = debug_shdr->sh_offset + debug_addr;
            debug_info_len = debug_shdr->sh_size;
        }
        else if (strn_cmp(string, ".debug_abbrev", 14) == 0)
            debug_abbrev = debug_shdr->sh_offset + debug_addr;
        else if (strn_cmp(string, ".debug_str", 11) == 0)
            debug_str = debug_shdr->sh_offset + debug_addr;
        else if (strn_cmp(string, ".debug_line_str", 16) == 0)
            debug_line_str = debug_shdr->sh_offset + debug_addr;
    }

    kdebug_outf("\nkcrash: info %x abbrev %x str %x %x", debug_info, debug_abbrev, debug_str, debug_line_str);
}

typedef struct
{
    uint32_t length;
    uint16_t version;
    uint8_t type;
    uint8_t addr_size;
    uint32_t abbrev_offset;
}__attribute__((packed)) compilation_unit;

typedef struct
{
    uint8_t type;
    uint8_t tag;
    uint8_t children;
    uint8_t data[];
}__attribute__((packed)) abbrev_block;

uint8_t kcrash_uleb128(uint8_t* num, uint64_t* value)
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

uint8_t kcrash_sleb128(uint8_t* num, int64_t* value)
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

char kcrash_file[128];
char kcrash_string[64];

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

void kcrash_checkcu(uint64_t compunit, uint64_t rip)
{
    compilation_unit *cu = (compilation_unit *)compunit;
    
    uint8_t *pointer = (uint8_t *)compunit;
    pointer += sizeof(compilation_unit);
    uint8_t *abbrev_pointer = (uint8_t *)(cu->abbrev_offset + (uint64_t)debug_abbrev);

    uint8_t level = 0;

    uint64_t size = 1;
    uint64_t line = 0, column = 0, high = 0, low = 0;

    while (1)
    {
        uint8_t abbrev_check = *pointer;
        uint64_t length = (uint64_t)pointer + cu->length;

        abbrev_block *block = (abbrev_block *)abbrev_pointer;
        uint64_t index = 0, uleb_num, leb_size;
        int64_t sleb_num = 0;
        
        while ((uint64_t)pointer < length)
        {
            block = (abbrev_block *)abbrev_pointer;
            size = 0;

            //if (abbrev_check == block->type)
            //    kdebug_outf("\nkdbg: <%x><%x>: Abbrev Num %d (%s)", level, (uint64_t)pointer - debug_info, abbrev_check, kdbg_tags[block->tag]);

            if ((abbrev_check == block->type) && (block->children == 1))
                level++;
            
            for (index = 0; ; index += 2)
            {
                if (abbrev_check == 0)
                    break;
                else if (block->data[index] == 0xB7 || block->data[index] == 0xB8 || block->data[index] == 0x82)
                {
                    index--;
                    continue;
                }
                else if (block->data[index] == 0x21)
                    index += 2;

                if (block->data[index] == 0x90)
                {
                    size += 1;
                    continue;
                }
                else if (block->data[index] == 0x91)
                {
                    size += 4;
                    continue;
                }

                //if (abbrev_check == block->type)
                //    kdebug_outf("\r\nkdbg:   <%x>  %s : %s", (uint64_t)pointer + size - debug_info, 
                //        kdbg_ats[block->data[index]], kdbg_forms[block->data[index + 1]]);
                
                uint8_t type = block->data[index + 1];
                switch (type)
                {
                    case 0x1E:
                        size += 16;
                        break;
                    case 0x1:
                        if (block->data[index] == 0x88)
                        {
                            index++;
                            size++;
                            break;
                        }
                        else if (block->data[index] == 0x83)
                        {
                            leb_size = kcrash_uleb128(pointer + size + 1, &uleb_num);
                            size += uleb_num + 1;
                            index++;
                            break;
                        }
                        else if (abbrev_check == block->type &&
                                (block->tag == 0x2E || block->tag == 0x11) &&
                                block->data[index] == 0x11)
                            low = *(uint64_t *)(pointer + size + 1);
                        size += cu->addr_size;
                        break;
                    case 0x7:
                    case 0x10:
                    case 0x14:
                    case 0x20:
                    case 0x24:
                        if (abbrev_check == block->type &&
                                (block->tag == 0x2E || block->tag == 0x11) &&
                                block->data[index] == 0x12)
                            high = *(uint64_t *)(pointer + size + 1);
                        size += 8;
                        break;
                    case 0xE:
                        if (abbrev_check == block->type)
                        {
                            char *strp = (char*)(*(uint32_t *)((uint64_t)pointer + size + 1) + debug_str);
                            if (block->tag == 0x11 && block->data[index] == 0x3)
                                memcpy(&kcrash_file, strp, str_len(strp) + 1);
                            else if (block->tag == 0x2E)
                                memcpy(&kcrash_string, strp, str_len(strp) + 1);
                        }
                        size += 4;
                        break;
                    case 0x1F:
                        if (abbrev_check == block->type)
                        {
                            char *strp = (char*)(*(uint32_t *)((uint64_t)pointer + size + 1) + debug_line_str);
                            if (block->tag == 0x11 && block->data[index] == 0x3)
                                memcpy(&kcrash_file, strp, str_len(strp) + 1);
                            else if (block->tag == 0x2E)
                                memcpy(&kcrash_string, strp, str_len(strp) + 1);
                        }
                        size += 4;
                        break;
                    case 0x4:
                    case 0x6:
                    case 0x13:
                    case 0x17:
                    case 0x1C:
                    case 0x1D:
                    case 0x28:
                    case 0x2C:
                        size += 4;
                        break;
                    case 0x27:
                    case 0x2B:
                        size += 3;
                        break;
                    case 0x3:
                    case 0x5:
                    case 0x12:
                    case 0x26:
                    case 0x2A:
                        if (abbrev_check == block->type &&
                            block->tag == 0x2E)
                        {
                            if (block->data[index] == 0x3B)
                                line = *(uint16_t*)(pointer+size+1);
                            else if (block->data[index] == 0x39)
                                column = *(uint16_t*)(pointer+size+1);
                        }
                        size += 2;
                        break;
                    case 0xA:
                    case 0xB:
                    case 0xC:
                    case 0x11:
                    case 0x25:
                    case 0x29:
                        if (abbrev_check == block->type &&
                            block->tag == 0x2E)
                        {
                            if (block->data[index] == 0x3B)
                                line = *(pointer + size + 1);
                            else if (block->data[index] == 0x39)
                                column = *(pointer + size + 1);
                        }
                        size++;
                        break;
                    case 0x9:
                    case 0xF:
                    case 0x15:
                    case 0x16:
                    case 0x1A:
                    case 0x1B:
                    case 0x22:
                    case 0x23:
                        leb_size = kcrash_uleb128((uint8_t *)((uint64_t)pointer + size + 1), &uleb_num);
                        size += leb_size;
                        if (abbrev_check == block->type &&
                                (block->tag == 0x2E || block->tag == 0x11) &&
                                block->data[index] == 0x12)
                            high = uleb_num;
                        break;
                    case 0xD:
                        leb_size = kcrash_sleb128(&block->data[index + 2], &sleb_num);
                        index -= 1;
                        size += leb_size;
                        if (abbrev_check == block->type &&
                            block->tag == 0x2E)
                        {
                            if (block->data[index] == 0x3B)
                                line = (uint64_t)sleb_num;
                            else if (block->data[index] == 0x39)
                                column = (uint64_t)sleb_num;
                        }
                    case 0x21:
                        leb_size = kcrash_sleb128(&block->data[index + 2], &sleb_num);
                        index += leb_size;
                        if (abbrev_check == block->type &&
                            block->tag == 0x2E)
                        {
                            if (block->data[index] == 0x3B)
                                line = (uint64_t)sleb_num;
                            else if (block->data[index] == 0x39)
                                column = (uint64_t)sleb_num;
                        }
                        break;
                    case 0x8:
                        char *str = (char*)((uint64_t)pointer + size + 1);
                        uint64_t str_size = str_len(str) + 1;
                        if (abbrev_check == block->type)
                        {
                            if (block->tag == 0x11)
                                memcpy(&kcrash_file, str, str_size);
                            else if (block->tag == 0x2E)
                                memcpy(&kcrash_string, str, str_size);
                        }
                        size += str_size;
                        break;
                    case 0x18:
                        leb_size = kcrash_uleb128((uint8_t *)((uint64_t)pointer + size + 1), &uleb_num);
                        size += uleb_num + 1;
                        break;
                    case 0x19:
                    case 0:
                        break;
                }
                if ((block->data[index] == 0) && (block->data[index + 1] == 0))
                    break;
            }
            abbrev_pointer += index + 5;
            if (abbrev_check == 0 && block->type == 0)
                level--;
            if (abbrev_check == block->type)
                break;
        }

        if (block->tag == 0x2E)
        {
            if ((rip >= low && rip <= low + high))
            {
                kdebug_outf(" (function:%s line:%d column:%d)", 
                    kcrash_string, line, column);
                if (screen)
                {
                    kscreen_putf(" (function:%s line:%d column:%d)", 
                        kcrash_string, line, column);
                }
                high = 0;
                low = 0;
                break;
            }
            high = 0;
            low = 0;
            memset(&kcrash_string, 0, 64);
        }
        else if ((rip >= low && rip <= low + high))
        {
            kdebug_outf(" file:%s", kcrash_file);
            if (screen)
                kscreen_putf(" file:%s", kcrash_file);
            high = 0;
            low = 0;
        }

        abbrev_pointer = (uint8_t *)(cu->abbrev_offset + (uint64_t)debug_abbrev);
        pointer = pointer + size + 1;

        if (level == 0)
        {
            size = 1;
            cu = (compilation_unit *)pointer;
            pointer += sizeof(compilation_unit);
            abbrev_pointer = (uint8_t *)(cu->abbrev_offset + (uint64_t)debug_abbrev);
            memset(&kcrash_file, 0, 64);
        }
        if ((uint64_t)pointer > debug_abbrev)
            break;
    }
}

void kcrash_debug(uint64_t rip)
{
    if (!debug_info || !debug_info_len || !debug_abbrev || !debug_str || !debug_line_str)
    {
        if (screen)
            kscreen_putf(" (no debug info found)");
        kdebug_outf(" (no debug info found)");
        return;
    }

    kcrash_checkcu(debug_info, rip);
}

#endif

uint8_t already_crashing = 0;

void kcrash(char *message)
{
    uint64_t rbp;
    __asm__ ("movq %%rbp, %0" : "=r"(rbp));

    screen = cw && ch;
    already_crashing++;

    if (screen)
        kscreen_putf("\n%n%m\n AQUA has crashed! \n Reason: [%s] \n", 0xFF0000, 0x0, message);
    kdebug_outf("\n\n AQUA has crashed! \n Reason: [%s] \n", message);

    struct kstackframe* stack = (struct kstackframe*)rbp;

    for (unsigned frame = 0; stack && frame < 8; ++frame)
    {
        if (screen)
            kscreen_putf("\nkcrash: [0x%x]", stack->rip);
        kdebug_outf("\nkcrash: [0x%x]", stack->rip);
#ifdef AQUA_DEBUG
        if (already_crashing < 2)
            kcrash_debug(stack->rip);
#endif

        stack = stack->rbp;
    }
    
    while (1)
        __asm__ ("hlt");
}