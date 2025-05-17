#include <kstring.h>
#include <stddef.h>
#include <debug.h>
#include <acpi.h>
#include <aml.h>
#include <mem.h>

uint8_t *aml_ptr = 0;

kacpi_field* kacpi_getfield(uint32_t length)
{
    kacpi_field *field = kmem_kalloc(sizeof(kacpi_field));
    field->elements = 0;
    kacpi_fieldelement *lastelement = 0;
    kacpi_fieldelement *element = 0;
    uint8_t *ptr = 0;
    uint32_t *ptr2 = 0;
    uint32_t len;

    size_t size = length - 1 + (uint64_t)aml_ptr;

    //aml_ptr++;

    while ((uint64_t)aml_ptr < size)
    {
        aml_ptr++;
        kdebug_outf(" field %x", *aml_ptr);
        
        switch (*aml_ptr)
        {
            case 0x00:
                element = kmem_kalloc(13);
                element->type = *aml_ptr;
                ptr2 = (uint32_t *)element->data;
                len = kacpi_getpkglength();
                ptr2[0] = len;
                break;
            case 0x01:
                element = kmem_kalloc(11);
                element->type = *aml_ptr;
                ptr = (uint8_t *)element->data;
                ptr[0] = kacpi_getbytedata();
                ptr[1] = kacpi_getbytedata();
                break;
            case 0x02:
                aml_ptr++;
                //if (*aml_ptr == buffertypeopcode)
                //  ...
                //else
                //  kacpi_getstringname
                break;
            case 0x03:
                element = kmem_kalloc(12);
                element->type = *aml_ptr;
                ptr = (uint8_t *)element->data;
                ptr[0] = kacpi_getbytedata();
                ptr[1] = kacpi_getbytedata();
                ptr[2] = kacpi_getbytedata();
                break;
            default:
                element = kmem_kalloc(17);
                element->type = 4;
                aml_ptr--;
                ptr = (uint8_t *)element->data;
                ptr2 = (uint32_t *)element->data;
                ptr[0] = kacpi_getbytedata();
                ptr[1] = kacpi_getbytedata();
                ptr[2] = kacpi_getbytedata();
                ptr[3] = kacpi_getbytedata();
                len = kacpi_getpkglength();
                ptr2[1] = len;
                break;
        }
        if (field->elements == 0)
        {
            field->elements = element;
            lastelement = element;
        }
        else
        {
            lastelement->next = (uint64_t *)element;
            lastelement = element;
        }
    }

    return field;
}

uint8_t* kacpi_getsupername()
{
    uint8_t* ptr;
    aml_ptr++;

    switch (*aml_ptr)
    {
        case 0x5B:
            aml_ptr++;
            ptr = kmem_kalloc(1);
            ptr[0] = *aml_ptr;
            break;
        default:
            if (*aml_ptr >= 0x60 && *aml_ptr <= 0x6E)
            {
                ptr = kmem_kalloc(1);
                ptr[0] = *aml_ptr;
            }
            else
            {
                ptr = kmem_kalloc(4);
                ptr[0] = *aml_ptr;
                ptr[1] = kacpi_getbytedata();
                ptr[2] = kacpi_getbytedata();
                ptr[3] = kacpi_getbytedata();
            }
            break;
    }

    return ptr;
}

kacpi_target* kacpi_gettarget()
{
    kacpi_target* target;

    aml_ptr++;

    if (*aml_ptr == 0)
    {
        target = kmem_kalloc(1);
        target->type = *aml_ptr;
    }
    else
    {
        target = kmem_kalloc(9);
        target->type = 1;
        aml_ptr--;
        target->string = kacpi_getsupername();
    }

    return target;
}

kacpi_termarg* kacpi_gettermarg()
{
    kacpi_termarg *termarg = 0;
    uint32_t len;
    
    aml_ptr++;

    if (*aml_ptr >= 0x60 && *aml_ptr <= 0x6E)
    {
        termarg = kmem_kalloc(1);
        termarg->type = *aml_ptr;
    }
    else
    {
        switch (*aml_ptr)
        {
            case 0x0A:
                termarg = kmem_kalloc(2);
                termarg->type = *aml_ptr;
                aml_ptr++;
                termarg->data[0] = *aml_ptr;
                break;
            case 0x0B:
                termarg = kmem_kalloc(3);
                termarg->type = *aml_ptr;
                aml_ptr++;
                termarg->data[0] = *aml_ptr;
                aml_ptr++;
                termarg->data[1] = *aml_ptr;
                break;
            case 0x0C:
                termarg = kmem_kalloc(5);
                termarg->type = *aml_ptr;
                for (int i = 0; i < 4; i++)
                {
                    aml_ptr++;
                    termarg->data[i] = *aml_ptr;
                }
                break;
            case 0x0D:
                aml_ptr++;
                len = str_len((char *)aml_ptr);
                termarg = kmem_kalloc(len + 1);
                termarg->type = *aml_ptr;
                memcpy(&termarg->data, aml_ptr, len);
                aml_ptr += len;
                break;
            case 0x0E:
                termarg = kmem_kalloc(9);
                termarg->type = *aml_ptr;
                for (int i = 0; i < 8; i++)
                {
                    aml_ptr++;
                    termarg->data[i] = *aml_ptr;
                }
                break;
            case 0x00:
            case 0x01:
            case 0xFF:
                termarg = kmem_kalloc(1);
                termarg->type = *aml_ptr;
                break;
            case 0x5B:
                termarg = kmem_kalloc(2);
                termarg->type = *aml_ptr;
                aml_ptr++;
                termarg->data[0] = *aml_ptr;
                break;
            case 0x11:
                len = kacpi_getpkglength();
                //size_t buffersize = 0;
                //kacpi_termarg *bufsz = kacpi_gettermarg();
                //figure this out later
                break;
        }
    }

    return termarg;
}

kacpi_termlist* kacpi_gettermlist()
{
    kacpi_termlist* list = kmem_kalloc(sizeof(kacpi_termlist));
    list->obj = 0;
    kacpi_termobj *lastobject = 0;
    kacpi_termobj *object = 0;
    uint8_t cont = 1;

    //Nothing | <termobj termlist>
    //termobj := Object | StatementOpcode | ExpressionOpcode

    //Object := NameSpaceModifierObj | NamedObj
    //StatementOpcode := 

    kdebug_outf(" TermList?");
    
    while (cont)
    {
        switch (*aml_ptr)
        {
            case 0x5B:
                kdebug_outf(" ExtOp");
                aml_ptr++;
                switch (*aml_ptr)
                {
                    case 0x80:
                        kdebug_outf(" OpRegionOp");
                        object = kmem_kalloc(sizeof(kacpi_defopregion));
                        kacpi_defopregion *opregion = (kacpi_defopregion *)object;
                        opregion->encodingvalue[0] = 0x5B;
                        opregion->encodingvalue[1] = *aml_ptr;
                        opregion->namestring = kacpi_getnamestring();
                        opregion->regionspace = kacpi_getbytedata();
                        opregion->regionoffset = kacpi_gettermarg();
                        opregion->regionlen = kacpi_gettermarg();
                        break;
                    case 0x81:
                        kdebug_outf(" FieldOp");
                        object = kmem_kalloc(sizeof(kacpi_deffield));
                        kacpi_deffield *field = (kacpi_deffield *)object;
                        field->encodingvalue[0] = 0x5B;
                        field->encodingvalue[1] = *aml_ptr;
                        field->pkglength = kacpi_getpkglength();
                        field->namestring = kacpi_getnamestring();
                        field->fieldflags = kacpi_getbytedata();
                        field->fieldlist = kacpi_getfield(field->pkglength);
                        break;
                    default:
                        kdebug_outf(" %x", *aml_ptr);
                        cont = 0;
                        break;
                }
                break;
            case 0x98:
                kdebug_outf(" ToHexStringOp");
                object = kmem_kalloc(sizeof(kacpi_deftohexstring));
                kacpi_deftohexstring *tohexstring = (kacpi_deftohexstring *)object;
                tohexstring->encodingvalue[0] = *aml_ptr;
                tohexstring->operand = kacpi_gettermarg();
                tohexstring->target = kacpi_gettarget();
                break;
            default:
                kdebug_outf(" %x", *aml_ptr);
                cont = 0;
                break;
        }
        if (list->obj == 0 && cont)
        {
            list->obj = object;
            lastobject = object;
        }
        else if (cont)
        {
            lastobject->next = (uint64_t *)object;
            lastobject = object;
        }
        aml_ptr++;
    }

    return list;
}

uint8_t kacpi_getbytedata()
{
    aml_ptr++;
    return *aml_ptr;
}

kacpi_namepath* kacpi_getnamepath()
{
    kacpi_namepath *namepath;
    uint8_t names = 1;

    aml_ptr++;
    
    if (*aml_ptr == 0x2E)
        names = 2;
    else if (*aml_ptr == 0x2F)
    {
        aml_ptr++;
        names = *aml_ptr;
    }
    else if (*aml_ptr == 0)
        names = 0;
    
    namepath = kmem_kalloc(1 + names * 4);
    namepath->names = names;

    aml_ptr++;
    memcpy(&namepath->name, aml_ptr, names * 4);
    aml_ptr += names * 4;

    return namepath;
}

kacpi_namestring* kacpi_getnamestring()
{
    kacpi_namestring *namestring = kmem_kalloc(sizeof(kacpi_namestring));
    
    aml_ptr++;

    if (*aml_ptr == '\\')
        namestring->first = '\\';
    else if (*aml_ptr == '^')
        namestring->first = '^'; //change this to looping until prefix ends, allocate string into struct
    else
        aml_ptr--;//nothing
    
    namestring->namepath = kacpi_getnamepath();
    
    return namestring;
}

uint32_t kacpi_getpkglength()
{
    uint32_t val = 0;

    aml_ptr++;

    uint8_t pkgleadbyte = *aml_ptr;
    uint8_t bytedata = pkgleadbyte >> 6;

    if (bytedata == 0)
        val = pkgleadbyte & 0x3F;
    else
    {
        val = pkgleadbyte & 0xF;
        for (int i = 0; i < bytedata; i++)
        {
            aml_ptr++;
            val = val << 8;
            val += *aml_ptr;
        }
    }
    return val;
}

void kacpi_printnamestring(kacpi_namestring *namestring)
{
    kdebug_outf(" name(s)");
    if (namestring->namepath->names == 0)
        kdebug_outf(" nullname");
    else
    {
        for (int i = 0; i < namestring->namepath->names; i++)
            kdebug_outf(" [%4s]", namestring->namepath->name[i]);
    }
}

void kacpi_printtermarg(kacpi_termarg *termarg)
{
    kdebug_outf(" termarg t%x", termarg->type);
    switch (termarg->type)
    {
        case 0x0:
            kdebug_outf(" ZeroOp");
            break;
        case 0x1:
            kdebug_outf(" OneOp");
            break;
        case 0xFF:
            kdebug_outf(" OnesOp");
            break;
    }
}

void kacpi_printfield(kacpi_field *field)
{
    kdebug_outf(" field");
    kacpi_fieldelement *element = field->elements;
    while (1)
    {
        switch (element->type)
        {
            case 0x00:
                kdebug_outf(" resv len %x", ((uint32_t *)element->data)[0]);
                break;
            case 0x01:
                kdebug_outf(" access type %x attrib %x", ((uint8_t *)element->data)[0], ((uint8_t *)element->data)[1]);
                break;
            case 0x02:
                kdebug_outf(" connect field");
                break;
            case 0x03:
                kdebug_outf(" ext access type %x ext attrib %x len %x", ((uint8_t *)element->data)[0], ((uint8_t *)element->data)[1], ((uint8_t *)element->data)[2]);
                break;
            default:
                kdebug_outf(" named %4s len %x", (char *)element->data, ((uint32_t *)element->data)[1]);
                break;
        }

        if (element->next == 0)
            break;
        element = (kacpi_fieldelement *)element->next;
    }
}

void kacpi_parseaml(uint8_t *aml, size_t length)
{
    kacpi_namespacemodifierobj *namespace = 0;
    aml_ptr = aml;
    kdebug_outf("\r\nkacpi: parsing aml, %x %x", (uintptr_t)aml_ptr, (uintptr_t)aml_ptr + 5);
    kdebug_outf("\r\n - hex output:");
    for (int i = 0; i < 64; i++)
    {
        kdebug_outf(" %2x", aml[i]);
    }
    //while ((uintptr_t)aml_ptr < (uintptr_t)aml_ptr + length)
    uint64_t end = (uintptr_t)aml_ptr + 5;
    while ((uintptr_t)aml_ptr < end)
    {
        switch (*aml_ptr)
        {
            case 0x00:
                kdebug_outf("\r\n - ZeroOp");
                break;
            case 0x10:
                kdebug_outf("\r\n - NameSpaceModifierObj op%x", *aml_ptr);
                // DefScope - ScopeOp PkgLength NameString TermList
                namespace = kmem_kalloc(sizeof(kacpi_namespacemodifierobj));
                namespace->op = *aml_ptr;
                kacpi_defscope *defscope = kmem_kalloc(sizeof(kacpi_defscope));
                namespace->def = (uint64_t *)defscope;
                defscope->pkglength = kacpi_getpkglength();
                defscope->namestring = kacpi_getnamestring();
                defscope->termlist = kacpi_gettermlist();
                break;
        }
        aml_ptr++;
    }

    kdebug_outf("\r\nkacpi: results");
    kdebug_outf("\r\nkacpi: namespace op%x", namespace->op);
    if (namespace->op == 0x10)
    {
        kacpi_defscope *defscope = (kacpi_defscope *)namespace->def;

        kdebug_outf("\r\n   - defscope pkglength %x", defscope->pkglength);
        kacpi_printnamestring(defscope->namestring);

        kacpi_termobj *obj = defscope->termlist->obj;
        while(1)
        {
            if (obj->encodingvalue[0] == 0x5B && obj->encodingvalue[1] == 0x80)
            {
                kacpi_defopregion *opregion = (kacpi_defopregion *)obj;
                kdebug_outf("\r\n     - opregion");
                kacpi_printnamestring(opregion->namestring);
                kdebug_outf(" regionspace %x", opregion->regionspace);
                kdebug_outf("\r\n       - regionoffset");
                kacpi_printtermarg(opregion->regionoffset);
                kdebug_outf("\r\n       - regionlen");
                kacpi_printtermarg(opregion->regionlen);
            }
            else if (obj->encodingvalue[0] == 0x5B && obj->encodingvalue[1] == 0x81)
            {
                kacpi_deffield *field = (kacpi_deffield *)obj;
                kdebug_outf("\r\n     - field pkglength %x", field->pkglength);
                kacpi_printnamestring(field->namestring);
                kdebug_outf(" fieldflags %x", field->fieldflags);
                kdebug_outf("\r\n      - fieldlist");
                kacpi_printfield(field->fieldlist);
            }

            if (obj->next == 0)
                break;
            obj = (kacpi_termobj *)obj->next;
        }
    }
}

void kacpi_processdsdt(acpi_dsdt *dsdt)
{
    kdebug_outf("\r\nkacpi: ** UNFINISHED - USE DSDT DATA LATER **");
    
    kdebug_outf("\r\nkacpi: processing DSDT table");
    uint8_t *aml = dsdt->aml;

    kdebug_outf("\r\nkacpi: aml data size %x", dsdt->h.length - sizeof(dsdt->h));
    kacpi_parseaml(aml, dsdt->h.length - sizeof(dsdt->h));

    kdebug_outf("\r\nkacpi: ** UNFINISHED - USE DSDT DATA LATER **");
}