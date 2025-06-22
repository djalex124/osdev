#include <stddef.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/aml.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

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

    size_t size = length + (uint64_t)aml_ptr;

    //aml_ptr++;

    while ((uint64_t)aml_ptr < size)
    {
        kdebug_outf(" field %2x", *aml_ptr);
        
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
                kdebug_outf(" (access)");
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
                element = kmem_kalloc(sizeof(kacpi_fieldelement_name));
                element->type = 4;
                kacpi_fieldelement_name *name = (kacpi_fieldelement_name*)element;
                aml_ptr++;
                kacpi_namepath *namepath = kacpi_getnamepath();
                memcpy(name->name, namepath->name[0], 4);
                aml_ptr++;
                name->pkglength = kacpi_getpkglength();
                break;
        }
        aml_ptr++;
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

    aml_ptr--;

    return field;
}

kacpi_supername* kacpi_getsupername()
{
    kacpi_supername* supername = kmem_kalloc(sizeof(kacpi_supername));

    if (*aml_ptr >= 0x60 && *aml_ptr <= 0x6E)
        supername->type = *aml_ptr;
    else if (*aml_ptr == 0x5B)
    {
        aml_ptr++;
        if (*aml_ptr == 0x31)
            supername->type = *aml_ptr;
    }
    else
    {
        switch (*aml_ptr)
        {
            case 0x71:
                supername->type = *aml_ptr;
                aml_ptr++;
                supername->data = (uint64_t *)kacpi_getsupername();
                break;
            case 0x83:
                supername->type = *aml_ptr;
                aml_ptr++;
                supername->data = (uint64_t *)kacpi_gettermarg();
                break;
            case 0x88:
                supername->type = *aml_ptr;
                aml_ptr++;
                supername->data = (uint64_t *)kacpi_gettermarg();
                break;
            default:
                supername->type = 1;
                supername->data = (uint64_t *)kacpi_getnamestring();
                break;
        }
    }

    return supername;
}

kacpi_target* kacpi_gettarget()
{
    kacpi_target* target = kmem_kalloc(sizeof(kacpi_target));

    if (*aml_ptr == 0)
        target->type = *aml_ptr;
    else
    {
        target->type = 1;
        target->data = (uint64_t *)kacpi_getsupername();
    }

    return target;
}

kacpi_termarg* kacpi_gettermarg()
{
    kacpi_termarg *termarg = 0;
    uint64_t *ptr = 0;
    uint32_t len;

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
            case 0x83:
                termarg = kmem_kalloc(9);
                termarg->type = *aml_ptr;
                aml_ptr++;
                ptr = (uint64_t *)termarg->data;
                *ptr = (uint64_t)kacpi_gettermarg();
                break;
            case 0x87:
                termarg = kmem_kalloc(9);
                termarg->type = *aml_ptr;
                aml_ptr++;
                kacpi_supername *supername = kacpi_getsupername();
                ptr = (uint64_t *)termarg->data;
                *ptr = (uint64_t)supername;
                break;
            case 0x88:
                termarg = kmem_kalloc(25);
                termarg->type = *aml_ptr;
                ptr = (uint64_t *)&termarg->data;
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg();
                ptr = (uint64_t *)&termarg->data[8];
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg();
                ptr = (uint64_t *)&termarg->data[16];
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettarget();
                break;
            case 0x95:
                termarg = kmem_kalloc(17);
                termarg->type = *aml_ptr;
                ptr = (uint64_t *)&termarg->data;
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg();
                ptr = (uint64_t *)&termarg->data[8];
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg();
                break;
            default:
                kdebug_outf(" uhta%2x ", *aml_ptr);
        }
    }

    return termarg;
}

uint64_t kacpi_getconst(uint8_t bytes)
{
    return 1;
}

kacpi_datarefobj* kacpi_getdatarefobj()
{
    kacpi_datarefobj *datarefobj = 0;

    uint32_t *dwordptr = 0;

    switch (*aml_ptr)
    {
        case 0:
            datarefobj = kmem_kalloc(1);
            datarefobj->type = *aml_ptr;
            break;
        case 0x0A:
            datarefobj = kmem_kalloc(2);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            datarefobj->data[0] = kacpi_getbytedata();
            break;
        case 0x0C:
            datarefobj = kmem_kalloc(5);
            datarefobj->type = *aml_ptr;
            dwordptr = (uint32_t *)&datarefobj->data;
            aml_ptr++;
            uint32_t dword = kacpi_getbytedata();
            aml_ptr++;
            dword += kacpi_getbytedata() << 8;
            aml_ptr++;
            dword += kacpi_getbytedata() << 16;
            aml_ptr++;
            dword += kacpi_getbytedata() << 24;
            *dwordptr = dword;
            break;
        default:
            kdebug_outf(" uhgdro %x", *aml_ptr);
            break;
    }

    return datarefobj;
}

kacpi_termlist* kacpi_gettermlist(uint32_t length)
{
    kacpi_termlist* list = kmem_kalloc(sizeof(kacpi_termlist));
    list->obj = 0;
    kacpi_termobj *lastobject = 0;
    kacpi_termobj *object = 0;
    uint64_t distance = 0;
    uint8_t cont = 1;

    //Nothing | <termobj termlist>
    //termobj := Object | StatementOpcode | ExpressionOpcode

    //Object := NameSpaceModifierObj | NamedObj
    //StatementOpcode := 

    kdebug_outf(" TermList?");
    
    size_t end = length + (uint64_t)aml_ptr - 1;

    while (cont)
    {
        switch (*aml_ptr)
        {
            case 0x08:
                kdebug_outf(" NameOp");
                object = kmem_kalloc(sizeof(kacpi_defname));
                kacpi_defname *name = (kacpi_defname *)object;
                name->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                name->namestring = kacpi_getnamestring();
                aml_ptr++;
                name->datarefobj = kacpi_getdatarefobj();
                break;
            case 0x14:
                kdebug_outf(" MethodOp");
                object = kmem_kalloc(sizeof(kacpi_defmethod));
                kacpi_defmethod *method = (kacpi_defmethod *)object;
                method->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                method->pkglength = kacpi_getpkglength();
                aml_ptr++;
                method->namestring = kacpi_getnamestring();
                aml_ptr++;
                method->methodflags = kacpi_getbytedata();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                method->termlist = kacpi_gettermlist(method->pkglength - distance);
                break;
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
                        aml_ptr++;
                        opregion->namestring = kacpi_getnamestring();
                        aml_ptr++;
                        opregion->regionspace = kacpi_getbytedata();
                        aml_ptr++;
                        opregion->regionoffset = kacpi_gettermarg();
                        aml_ptr++;
                        opregion->regionlen = kacpi_gettermarg();
                        break;
                    case 0x81:
                        kdebug_outf(" FieldOp");
                        object = kmem_kalloc(sizeof(kacpi_deffield));
                        kacpi_deffield *field = (kacpi_deffield *)object;
                        field->encodingvalue[0] = 0x5B;
                        field->encodingvalue[1] = *aml_ptr;
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr;
                        field->pkglength = kacpi_getpkglength();
                        aml_ptr++;
                        field->namestring = kacpi_getnamestring();
                        aml_ptr++;
                        field->fieldflags = kacpi_getbytedata();
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr - distance;
                        field->fieldlist = kacpi_getfield(field->pkglength - distance);
                        break;
                    case 0x82:
                        kdebug_outf(" DeviceOp");
                        object = kmem_kalloc(sizeof(kacpi_defdevice));
                        kacpi_defdevice *device = (kacpi_defdevice *)object;
                        device->encodingvalue[0] = 0x5B;
                        device->encodingvalue[1] = *aml_ptr;
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr;
                        device->pkglength = kacpi_getpkglength();
                        aml_ptr++;
                        device->namestring = kacpi_getnamestring();
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr - distance;
                        device->termlist = kacpi_gettermlist(device->pkglength - distance);
                        break;
                    default:
                        kdebug_outf(" %x", *aml_ptr);
                        cont = 0;
                        break;
                }
                break;
            case 0x70:
                kdebug_outf(" StoreOp");
                object = kmem_kalloc(sizeof(kacpi_defstore));
                kacpi_defstore *store = (kacpi_defstore *)object;
                store->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                store->operand = kacpi_gettermarg();
                aml_ptr++;
                store->supername = kacpi_getsupername();
                break;
            case 0x74:
                kdebug_outf(" SubtractOp");
                object = kmem_kalloc(sizeof(kacpi_defsubtract));
                kacpi_defsubtract *subtract = (kacpi_defsubtract *)object;
                subtract->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                subtract->operand1 = kacpi_gettermarg();
                aml_ptr++;
                subtract->operand2 = kacpi_gettermarg();
                aml_ptr++;
                subtract->target = kacpi_gettarget();
                break;
            case 0x75:
                kdebug_outf(" IncrementOp");
                object = kmem_kalloc(sizeof(kacpi_defincrement));
                kacpi_defincrement *increment = (kacpi_defincrement *)object;
                increment->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                increment->supername = kacpi_getsupername();
                break;
            case 0x96:
                kdebug_outf(" ToBufferOp");
                object = kmem_kalloc(sizeof(kacpi_deftobuffer));
                kacpi_deftobuffer *tobuffer = (kacpi_deftobuffer *)object;
                tobuffer->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                tobuffer->operand = kacpi_gettermarg();
                aml_ptr++;
                tobuffer->target = kacpi_gettarget();
                break;
            case 0x98:
                kdebug_outf(" ToHexStringOp");
                object = kmem_kalloc(sizeof(kacpi_deftohexstring));
                kacpi_deftohexstring *tohexstring = (kacpi_deftohexstring *)object;
                tohexstring->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                tohexstring->operand = kacpi_gettermarg();
                aml_ptr++;
                tohexstring->target = kacpi_gettarget();
                break;
            case 0xA2:
                kdebug_outf(" DefWhile");
                object = kmem_kalloc(sizeof(kacpi_defwhile));
                kacpi_defwhile *dwhile = (kacpi_defwhile *)object;
                dwhile->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                dwhile->pkglength = kacpi_getpkglength();
                aml_ptr++;
                dwhile->predicate = kacpi_gettermarg();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                dwhile->termlist = kacpi_gettermlist(dwhile->pkglength - distance);
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
        if ((uint64_t)aml_ptr < end)
            aml_ptr++;
        else
            break;
    }

    return list;
}

uint8_t kacpi_getbytedata()
{
    uint8_t v = *aml_ptr;
    return v;
}

kacpi_namepath* kacpi_getnamepath()
{
    kacpi_namepath *namepath;
    uint8_t names = 1;
    
    if (*aml_ptr == 0x2E)
        names = 2;
    else if (*aml_ptr == 0x2F)
    {
        aml_ptr++;
        names = *aml_ptr;
    }
    else if (*aml_ptr == 0)
    {
        namepath = kmem_kalloc(1);
        namepath->names = 0;
        return namepath;
    }
    else
        aml_ptr--;
    
    namepath = kmem_kalloc(1 + names * 4);
    namepath->names = names;

    kdebug_outf(" %x%x%x%x", *aml_ptr, *(aml_ptr+1), *(aml_ptr+2), *(aml_ptr+3));

    memcpy(&namepath->name, aml_ptr, names * 4);
    aml_ptr += names * 4;

    aml_ptr--;

    return namepath;
}

kacpi_namestring* kacpi_getnamestring()
{
    kacpi_namestring *namestring = kmem_kalloc(sizeof(kacpi_namestring));

    //kdebug_outf(" %x%x%x%x", *aml_ptr, *(aml_ptr+1), *(aml_ptr+2), *(aml_ptr+3));

    if (*aml_ptr == '\\')
        namestring->first = '\\';
    else if (*aml_ptr == '^')
        namestring->first = '^'; //change this to looping until prefix ends, allocate string into struct

    aml_ptr++;
    
    namestring->namepath = kacpi_getnamepath();
    
    return namestring;
}

uint32_t kacpi_getpkglength()
{
    uint32_t val = 0;

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
            val += (*aml_ptr << (4 + 8*i));
        }
    }
    return val;
}

uint8_t depth = 0;

void kacpi_printnamestring(kacpi_namestring *namestring)
{
    kdebug_outf("\r\n[%d] name(s)", depth);
    if (namestring->namepath->names == 0)
        kdebug_outf(" nullname");
    else
    {
        for (int i = 0; i < namestring->namepath->names; i++)
            kdebug_outf(" [%4s]", namestring->namepath->name[i]);
    }
}

void kacpi_printsupername(kacpi_supername *supername)
{
    if (supername->type >= 0x60 && supername->type <= 0x67)
        kdebug_outf(" Local%dOp", supername->type - 0x60);
    else if (supername->type >= 0x68 && supername->type <= 0x6E)
        kdebug_outf(" Arg%dOp", supername->type - 0x68);
    else if (supername->type == 1)
        kacpi_printnamestring((kacpi_namestring *)supername->data);
}

void kacpi_printtarget(kacpi_target *target)
{
    kdebug_outf("\r\n[%d] target", depth);
    if (target->type == 0)
        kdebug_outf(" Null");
    else
    {
        kacpi_supername *supername = (kacpi_supername *)target->data;
        kacpi_printsupername(supername);
    }
}

void kacpi_printtermarg(kacpi_termarg *termarg)
{
    kdebug_outf("\r\n[%d] termarg", depth);
    if (termarg->type >= 0x60 && termarg->type <= 0x67)
        kdebug_outf(" Local%dOp", termarg->type - 0x60);
    else if (termarg->type >= 0x68 && termarg->type <= 0x6E)
        kdebug_outf(" Arg%dOp", termarg->type - 0x68);
    else
    {
        depth++;
        switch (termarg->type)
        {
            case 0x0:
                kdebug_outf(" ZeroOp");
                break;
            case 0x1:
                kdebug_outf(" OneOp");
                break;
            case 0x0A:
                kdebug_outf(" Byte, 0x%x", termarg->data[0]);
                break;
            case 0x0B:
                kdebug_outf(" Word, 0x%x", termarg->data[0] + (termarg->data[1] << 8));
                break;
            case 0x83:
                kdebug_outf(" DerefOf");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&termarg->data[0]));
                break;
            case 0x87:
                kdebug_outf(" SizeOf");
                kacpi_supername *supername = (kacpi_supername *)(*(uint64_t *)&termarg->data[0]);
                kacpi_printsupername(supername);
                break;
            case 0x88:
                kdebug_outf(" IndexOf");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&termarg->data[0]));
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&termarg->data[8]));
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&termarg->data[16]));
                break;
            case 0x95:
                kdebug_outf(" L_Less");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&termarg->data[0]));
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&termarg->data[8]));
                break;
            case 0xFF:
                kdebug_outf(" OnesOp");
                break;
            default:
                kdebug_outf(" other %2x", termarg->type);
                break;
        }
        depth--;
    }
}

void kacpi_printfield(kacpi_field *field)
{
    kdebug_outf("\r\n[%d] field", depth);
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
            case 0x04:
                kacpi_fieldelement_name *name = (kacpi_fieldelement_name *)element;
                kdebug_outf(" named %4s len %x", name->name, name->pkglength);
                break;
            default:
                kdebug_outf(" named %4s len %x", ((kacpi_namepath *)element->data)->name, ((uint32_t *)element->data)[1]);
                break;
        }

        if (element->next == 0)
            break;
        element = (kacpi_fieldelement *)element->next;
    }
}

void kacpi_printdatarefobj(kacpi_datarefobj *obj)
{
    kdebug_outf("\r\n[%d] datarefobj", depth);
    switch (obj->type)
    {
        case 0:
            kdebug_outf(" [zero]");
            break;
        case 0xC:
            kdebug_outf(" [%8x]", *(uint32_t *)obj->data);
            break;
    }
}

void kacpi_printtermlist(kacpi_termlist *list)
{
    kdebug_outf("\r\n[%d] termlist", depth);
    kacpi_termobj *obj = list->obj;
    depth++;
    while ((uint64_t)obj)
    {
        switch (obj->encodingvalue[0])
        {
            case 0x08:
                kdebug_outf("\r\n[%d] name", depth);
                depth++;
                kacpi_defname *name = (kacpi_defname *)obj;
                kacpi_printnamestring(name->namestring);
                kacpi_printdatarefobj(name->datarefobj);
                break;
            case 0x70:
                kdebug_outf("\r\n[%d] store", depth);
                depth++;
                kacpi_defstore *store = (kacpi_defstore *)obj;
                kacpi_printtermarg(store->operand);
                kacpi_printsupername(store->supername);
                break;
            case 0x74:
                kdebug_outf("\r\n[%d] subtract", depth);
                depth++;
                kacpi_defsubtract *subtract = (kacpi_defsubtract *)obj;
                kacpi_printtermarg(subtract->operand1);
                kacpi_printtermarg(subtract->operand2);
                kacpi_printtarget(subtract->target);
                break;
            case 0x75:
                kdebug_outf("\r\n[%d] increment", depth);
                depth++;
                kacpi_defincrement *increment = (kacpi_defincrement *)obj;
                kacpi_printsupername(increment->supername);
                break;
            case 0x96:
                kdebug_outf("\r\n[%d] tobuffer", depth);
                depth++;
                kacpi_deftobuffer *tobuffer = (kacpi_deftobuffer *)obj;
                kacpi_printtermarg(tobuffer->operand);
                kacpi_printtarget(tobuffer->target);
                break;
            case 0x98:
                kdebug_outf("\r\n[%d] tohexstring", depth);
                depth++;
                kacpi_deftohexstring *tohexstring = (kacpi_deftohexstring *)obj;
                kacpi_printtermarg(tohexstring->operand);
                kacpi_printtarget(tohexstring->target);
                break;
            case 0xA2:
                kdebug_outf("\r\n[%d] while", depth);
                depth++;
                kacpi_defwhile *dwhile = (kacpi_defwhile *)obj;
                kdebug_outf("\r\n[%d] pkglength %x", depth, dwhile->pkglength);
                kacpi_printtermarg(dwhile->predicate);
                kacpi_printtermlist(dwhile->termlist);
                break;
        }
        depth--;
        obj = (kacpi_termobj *)obj->next;
    }
    depth--;
}

kacpi_namespacemodifierobj *namespace = 0;
uint8_t namespaces = 0;

void kacpi_printaml()
{
    kdebug_outf("\r\nkacpi: results");

    kacpi_namespacemodifierobj *ns = namespace;
    
    for (int i = 0; i < namespaces; i++)
    {
        if (ns->op == 0x10)
        {
            kacpi_defscope *defscope = (kacpi_defscope *)ns->def;

            kdebug_outf("\r\n[%d] defscope pkglength %x", depth, defscope->pkglength);
            kacpi_printnamestring(defscope->namestring);

            depth++;

            kacpi_termobj *obj = defscope->termlist->obj;
            while(1)
            {
                if (obj->encodingvalue[0] == 0x5B && obj->encodingvalue[1] == 0x80)
                {
                    kacpi_defopregion *opregion = (kacpi_defopregion *)obj;
                    kdebug_outf("\r\n[%d] opregion", depth);
                    depth++;
                    kacpi_printnamestring(opregion->namestring);
                    kdebug_outf(" regionspace %x", opregion->regionspace);
                    kacpi_printtermarg(opregion->regionoffset);
                    kacpi_printtermarg(opregion->regionlen);
                    depth--;
                }
                else if (obj->encodingvalue[0] == 0x5B && obj->encodingvalue[1] == 0x81)
                {
                    kacpi_deffield *field = (kacpi_deffield *)obj;
                    kdebug_outf("\r\n[%d] field pkglength %x", depth, field->pkglength);
                    depth++;
                    kacpi_printnamestring(field->namestring);
                    kdebug_outf(" fieldflags %x", field->fieldflags);
                    kacpi_printfield(field->fieldlist);
                    depth--;
                }
                else if (obj->encodingvalue[0] == 0x5B && obj->encodingvalue[1] == 0x82)
                {
                    kacpi_defdevice *device = (kacpi_defdevice *)obj;
                    kdebug_outf("\r\n[%d] device pkglength %x", depth, device->pkglength);
                    depth++;
                    kacpi_printnamestring(device->namestring);
                    kacpi_printtermlist(device->termlist);
                    depth--;
                }
                else if (obj->encodingvalue[0] == 0x14)
                {
                    kacpi_defmethod *method = (kacpi_defmethod *)obj;
                    kdebug_outf("\r\n[%d] method pkglength %x", depth, method->pkglength);
                    depth++;
                    kacpi_printnamestring(method->namestring);
                    kdebug_outf(" methodflags %x", method->methodflags);
                    kacpi_printtermlist(method->termlist);
                    depth--;
                }

                if (obj->next == 0)
                    break;
                obj = (kacpi_termobj *)obj->next;
            }
        }
        ns = (kacpi_namespacemodifierobj *)ns->next;
    }
}

void kacpi_parseaml(uint8_t *aml, size_t length)
{
    aml_ptr = aml;
    kdebug_outf("\r\nkacpi: parsing aml, %x %x", (uintptr_t)aml_ptr, (uintptr_t)aml_ptr + 5);
    kdebug_outf("\r\n - hex output:");
    for (int i = 0; i < 256; i++)
    {
        kdebug_outf(" %2x", aml[i]);
    }

    //while ((uintptr_t)aml_ptr < (uintptr_t)aml_ptr + length)
    uint64_t end = (uintptr_t)aml_ptr + 150;
    while ((uintptr_t)aml_ptr < end)
    {
        switch (*aml_ptr)
        {
            case 0x00:
                kdebug_outf("\r\n - ZeroOp");
                break;
            case 0x10:
                kacpi_namespacemodifierobj *ns = kmem_kalloc(sizeof(kacpi_namespacemodifierobj));
                kdebug_outf("\r\n - NameSpaceModifierObj op%x", *aml_ptr);
                // DefScope - ScopeOp PkgLength NameString TermList
                ns->op = *aml_ptr;
                kacpi_defscope *defscope = kmem_kalloc(sizeof(kacpi_defscope));
                ns->def = (uint64_t *)defscope;
                namespaces++;
                aml_ptr++;
                uint64_t distance = (uint64_t)aml_ptr;
                defscope->pkglength = kacpi_getpkglength();
                aml_ptr++;
                defscope->namestring = kacpi_getnamestring();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                defscope->termlist = kacpi_gettermlist(defscope->pkglength - distance);
                if ((uint64_t)namespace)
                {
                    kacpi_namespacemodifierobj *next = namespace;
                    for (int i = 0; i < namespaces - 1; i++)
                        next = (kacpi_namespacemodifierobj *)next->next;
                    next->next = (uint64_t *)ns;
                }
                else
                    namespace = ns;
                break;
        }
        aml_ptr++;
    }

    //commented for now as it just spams console
    //kacpi_printaml();
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