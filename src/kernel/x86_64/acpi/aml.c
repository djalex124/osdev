#include <stddef.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/aml.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

uint8_t *aml_ptr = 0;

kacpi_termlist *kacpi_dsdt = 0;

kacpi_fieldlist* kacpi_getfield(uint32_t length)
{
    kacpi_fieldlist *fieldlist = kmem_kalloc(sizeof(kacpi_fieldlist));
    kacpi_fieldlist *fieldlistptr = fieldlist;

    size_t end = length + (uint64_t)aml_ptr - 1;

    while ((uint64_t)aml_ptr < end)
    {
        kdebug_outf(" field %2x", *aml_ptr);
        
        kacpi_fieldelement *fieldelement = 0;
        switch (*aml_ptr)
        {
            case 0x00:
                fieldelement = kmem_kalloc(5);
                fieldelement->type = *aml_ptr;
                aml_ptr++;
                ((uint32_t *)fieldelement->data)[0] = kacpi_getpkglength();
                break;
            case 0x01:
                fieldelement = kmem_kalloc(3);
                fieldelement->type = *aml_ptr;
                aml_ptr++;
                fieldelement->data[0] = kacpi_getbytedata();
                aml_ptr++;
                fieldelement->data[1] = kacpi_getbytedata();
                break;
            case 0x02:
                kdebug_outf(" element 0x02");
                asm("hlt");
                break;
            case 0x03:
                fieldelement = kmem_kalloc(4);
                fieldelement->type = *aml_ptr;
                aml_ptr++;
                fieldelement->data[0] = kacpi_getbytedata();
                aml_ptr++;
                fieldelement->data[1] = kacpi_getbytedata();
                aml_ptr++;
                fieldelement->data[2] = kacpi_getbytedata();
                break;
            default:
                fieldelement = kmem_kalloc(13);
                fieldelement->type = 4;
                ((uint64_t *)fieldelement->data)[0] = (uint64_t)kacpi_getnamepath();
                aml_ptr++;
                ((uint32_t *)fieldelement->data)[2] = kacpi_getpkglength();
                break;
        }
        fieldlistptr->element = fieldelement;
        fieldlistptr->next = kmem_kalloc(sizeof(kacpi_fieldlist));
        fieldlistptr = (kacpi_fieldlist *)fieldlistptr->next;
        aml_ptr++;
    }

    aml_ptr = (uint8_t *)end;

    return fieldlist;
}

kacpi_supername* kacpi_getsupername(uint64_t *parent)
{
    kacpi_supername* supername = kmem_kalloc(sizeof(kacpi_supername));

    kdebug_outf("[ snta%2x", *aml_ptr);

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
                supername->data = (uint64_t *)kacpi_getsupername(0);
                break;
            case 0x83:
            case 0x88:
                supername->type = *aml_ptr;
                supername->data = (uint64_t *)kacpi_gettermarg(0);
                break;
            default:
                supername->type = 1;
                supername->data = (uint64_t *)kacpi_gettermarg(parent);
                break;
        }
    }

    kdebug_outf("]");

    return supername;
}

kacpi_target* kacpi_gettarget(uint64_t *parent)
{
    kacpi_target* target = kmem_kalloc(sizeof(kacpi_target));

    if (*aml_ptr == 0)
    {
        target->type = *aml_ptr;
        target->data = 0;
    }
    else
    {
        target->type = 1;
        target->data = (uint64_t *)kacpi_getsupername(parent);
    }

    return target;
}

uint64_t kacpi_termarginteger(kacpi_termarg *arg)
{
    uint64_t constant = 0;

    switch (arg->type)
    {
        case 0x0A:
            constant = (uint64_t)arg->data[0];
            break;
        case 0x0B:
            constant = (uint64_t)arg->data[0];
            constant += (uint64_t)arg->data[1] << 8;
            break;
        case 0x0C:
            constant = (uint64_t)arg->data[0];
            constant += (uint64_t)arg->data[1] << 8;
            constant += (uint64_t)arg->data[2] << 16;
            constant += (uint64_t)arg->data[3] << 24;
            break;
        case 0x00:
        case 0x01:
            constant = arg->type;
            break;
        default:
            kdebug_outf(" tai fail! %2x", arg->type);
            break;
    }

    return constant;
}

uint8_t kacpi_depth = 0;

kacpi_expression* kacpi_getexpression(uint64_t *parent)
{
    kacpi_expression *expression = kmem_kalloc(2);
    expression->encodingvalue[0] = *aml_ptr;

    uint64_t *ptr = (uint64_t *)&expression->data[0];
    uint64_t distance = 0;

    kacpi_defmethod *method = 0;

    kdebug_outf(" EX:%2x", *aml_ptr);

    switch (*aml_ptr)
    {
        case 0x11:
            kmem_kalloc(sizeof(kacpi_buffer) - 2);
            aml_ptr++;
            kacpi_buffer *buffer = (kacpi_buffer *)expression;
            buffer->pkglength = kacpi_getpkglength();
            aml_ptr++;
            buffer->buffersize = kacpi_gettermarg(parent);
            aml_ptr++;
            uint32_t len = kacpi_termarginteger(buffer->buffersize);
            uint8_t* buf = kmem_kalloc(len);
            memcpy(buf, aml_ptr, len);
            buffer->bytelist = buf;
            aml_ptr += len - 1;
            break;
        case 0x12:
            kmem_kalloc(sizeof(kacpi_defpackageop) - 2);
            aml_ptr++;
            kacpi_defpackageop *defpackage = (kacpi_defpackageop *)expression;
            distance = (uint64_t)aml_ptr;
            defpackage->pkglength = kacpi_getpkglength();
            aml_ptr++;
            defpackage->varnumelements = kacpi_getbytedata();
            aml_ptr++;
            distance = (uint64_t)aml_ptr - distance;
            defpackage->packageelementlist = kacpi_getpackageelementlist(
                defpackage->pkglength - distance, defpackage->varnumelements);
            break;
        case 0x13:
            kmem_kalloc(sizeof(kacpi_defvarpackageop) - 2);
            aml_ptr++;
            kacpi_defvarpackageop *varpackageop = (kacpi_defvarpackageop *)expression;
            aml_ptr++;
            distance = (uint64_t)aml_ptr;
            varpackageop->pkglength = kacpi_getpkglength();
            aml_ptr++;
            varpackageop->varnumelements = kacpi_gettermarg(parent);
            aml_ptr++;
            distance = (uint64_t)aml_ptr - distance;
            varpackageop->packageelementlist = kacpi_getpackageelementlist(
                varpackageop->pkglength - distance, kacpi_termarginteger(varpackageop->varnumelements));
            break;
        case 0x70:
            kmem_kalloc(16);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            ptr = (uint64_t *)&expression->data[8];
            aml_ptr++;
            *ptr = (uint64_t)kacpi_getsupername(parent);
            break;
        case 0x75:
        case 0x76:
            kmem_kalloc(8);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_getsupername(parent);
            break;
        case 0x83:
            kmem_kalloc(8);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            break;
        case 0x87:
            kmem_kalloc(8);
            aml_ptr++;
            kacpi_supername *supername = kacpi_getsupername(parent);
            *ptr = (uint64_t)supername;
            break;
        case 0x72 ... 0x74:
        case 0x79 ... 0x7F:
        case 0x88:
            kmem_kalloc(24);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            ptr = (uint64_t *)&expression->data[8];
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            ptr = (uint64_t *)&expression->data[16];
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettarget(parent);
            break;
        case 0x92:
            kmem_kalloc(8);
            aml_ptr++;
            if (*aml_ptr == 0x93 || *aml_ptr == 0x94 || *aml_ptr == 0x95)
            {
                kmem_kalloc(16);
                *ptr = *aml_ptr;
                ptr = (uint64_t *)&expression->data[8];
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg(parent);
                ptr = (uint64_t *)&expression->data[16];
                aml_ptr++;
                *ptr = (uint64_t)kacpi_gettermarg(parent);
                break;
            }
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            break;
        case 0x90:
        case 0x91:
        case 0x93 ... 0x95:
            kmem_kalloc(16);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            ptr = (uint64_t *)&expression->data[8];
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            break;
        case 0x96 ... 0x99:
            kmem_kalloc(16);
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettermarg(parent);
            ptr = (uint64_t *)&expression->data[8];
            aml_ptr++;
            *ptr = (uint64_t)kacpi_gettarget(parent);
            break;
        case 0x5B:
            aml_ptr++;
            expression->encodingvalue[1] = *aml_ptr;
            kdebug_outf(" %2x", *aml_ptr);
            switch (*aml_ptr)
            {
                case 0x01:
                    kmem_kalloc(16);
                    aml_ptr++;
                    *ptr = (uint64_t)kacpi_getnamestring();
                    ptr = (uint64_t *)&expression->data[8];
                    aml_ptr++;
                    *ptr = kacpi_getbytedata();
                    break;
                case 0x23:
                    kmem_kalloc(16);
                    aml_ptr++;
                    *ptr = (uint64_t)kacpi_getsupername(parent);
                    ptr = (uint64_t *)&expression->data[8];
                    aml_ptr++;
                    distance = kacpi_getbytedata();
                    aml_ptr++;
                    distance += kacpi_getbytedata() << 8;
                    *ptr = distance;
                    break;
                case 0x81:
                    kmem_kalloc(sizeof(kacpi_deffield) - 2);
                    kacpi_deffield *field = (kacpi_deffield *)expression;
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
                    kmem_kalloc(sizeof(kacpi_defdevice) - 2);
                    kacpi_defdevice *device = (kacpi_defdevice *)expression;
                    aml_ptr++;
                    distance = (uint64_t)aml_ptr;
                    device->pkglength = kacpi_getpkglength();
                    aml_ptr++;
                    device->namestring = kacpi_getnamestring();
                    kdebug_outf(" namepath %dnames [%4s]", 
                        device->namestring->namepath->names,
                        device->namestring->namepath->name[0]);
                    aml_ptr++;
                    distance = (uint64_t)aml_ptr - distance;
                    kacpi_gettermlist(device->pkglength - distance, parent, &device->termlist);
                    break;
            }
            break;
        default:
            kmem_kalloc(16);
            expression->encodingvalue[0] = 0xFD;
            kacpi_namestring *namestring = kacpi_getnamestring();
            *ptr = (uint64_t)namestring;

            if (namestring->first != '\\')
            {
                //get full name and scope
            }

            //need to store everything based on scope, device, name

            if (expression->encodingvalue[0] == 0xFE)
            {
                method = (kacpi_defmethod *)0;
                *ptr = (uint64_t)method;
                kacpi_termarglist *termarglist = kmem_kalloc(sizeof(kacpi_termarglist));
                ptr = (uint64_t *)&expression->data[8];
                *ptr = (uint64_t)termarglist;
                kdebug_outf(" args%d", method->methodflags & 0b111);
                for (uint8_t args = method->methodflags & 0b111; args > 0; args--)
                {
                    aml_ptr++;
                    termarglist->arg = kacpi_gettermarg(0);
                    kacpi_termarglist *next = kmem_kalloc(sizeof(kacpi_termarglist));
                    termarglist->next = (uint64_t *)next;
                    termarglist = next;
                }
            }

            break;
    }

    return expression;
}

void kacpi_gettermlist(uint32_t length, uint64_t *parent, kacpi_termlist **returnlist)
{
    if (length == 0)
    {
        aml_ptr--;
        *returnlist = 0;
        return;
    }

    kacpi_termlist *list = kmem_kalloc(sizeof(kacpi_termlist));
    *returnlist = list;
    kacpi_termlist *listptr = list;
    uint64_t distance = 0;
    uint8_t cont = 1;

    kdebug_outf("\nTL:");
    
    size_t end = length + (uint64_t)aml_ptr - 1;

    while (cont)
    {
        kacpi_expression *object = 0;
        kacpi_depth++;
        listptr->depth = kacpi_depth;
        listptr->parent = parent;
        switch (*aml_ptr)
        {
            case 0x06:
                object = kmem_kalloc(sizeof(kacpi_defalias));
                listptr->obj = object;
                kacpi_defalias *alias = (kacpi_defalias *)object;
                alias->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                alias->namestring1 = kacpi_getnamestring();
                aml_ptr++;
                alias->namestring2 = kacpi_getnamestring();
                break;
            case 0x08:
                object = kmem_kalloc(sizeof(kacpi_defname));
                listptr->obj = object;
                kacpi_defname *name = (kacpi_defname *)object;
                name->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                name->namestring = kacpi_getnamestring();
                aml_ptr++;
                name->datarefobj = kacpi_getdatarefobj();
                break;
            case 0x10:
                object = kmem_kalloc(sizeof(kacpi_defscope));
                listptr->obj = object;
                kacpi_defscope *scope = (kacpi_defscope *)object;
                scope->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                scope->pkglength = kacpi_getpkglength();
                aml_ptr++;
                scope->namestring = kacpi_getnamestring();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                kacpi_gettermlist(scope->pkglength - distance, (uint64_t *)listptr, &scope->termlist);
                break;
            case 0x14:
                object = kmem_kalloc(sizeof(kacpi_defmethod));
                listptr->obj = object;
                kacpi_defmethod* method = (kacpi_defmethod *)object;
                method->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                method->pkglength = kacpi_getpkglength();
                aml_ptr++;
                method->namestring = kacpi_getnamestring();
                kdebug_outf("\n\nmethod named ");
                kacpi_printnamestring(method->namestring);
                aml_ptr++;
                method->methodflags = kacpi_getbytedata();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                kacpi_gettermlist(method->pkglength - distance, (uint64_t *)listptr, &method->termlist);
                break;
            case 0x86:
                object = kmem_kalloc(sizeof(kapci_defnotify));
                listptr->obj = object;
                kapci_defnotify* notify = (kapci_defnotify *)object;
                notify->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                notify->notifyobject = kacpi_getsupername((uint64_t *)listptr);
                aml_ptr++;
                notify->notifyvalue = kacpi_gettermarg((uint64_t *)listptr);
                break;
            case 0x8A:
                object = kmem_kalloc(sizeof(kacpi_defcreatedwordfield));
                listptr->obj = object;
                kacpi_defcreatedwordfield* createdwordfield = (kacpi_defcreatedwordfield *)object;
                createdwordfield->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                createdwordfield->buffer = kacpi_gettermarg((uint64_t *)listptr);
                aml_ptr++;
                createdwordfield->byteindex = kacpi_gettermarg((uint64_t *)listptr);
                aml_ptr++;
                createdwordfield->name = kacpi_getnamestring();
                break;
            case 0xA0:
                kdebug_outf(" DefIfOp");
                object = kmem_kalloc(sizeof(kacpi_defifop));
                listptr->obj = object;
                kacpi_defifop *dif = (kacpi_defifop *)object;
                dif->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                dif->pkglength = kacpi_getpkglength();
                aml_ptr++;
                dif->predicate = kacpi_gettermarg((uint64_t *)listptr);
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                kacpi_gettermlist(dif->pkglength - distance, (uint64_t *)listptr, &dif->termlist);
                break;
            case 0xA1:
                kdebug_outf(" DefElseOp");
                object = kmem_kalloc(sizeof(kacpi_defelseop));
                listptr->obj = object;
                kacpi_defelseop *delse = (kacpi_defelseop *)object;
                delse->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                delse->pkglength = kacpi_getpkglength();
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                kacpi_gettermlist(delse->pkglength - distance, (uint64_t *)listptr, &delse->termlist);
                break;
            case 0xA2:
                kdebug_outf(" DefWhile");
                object = kmem_kalloc(26);
                listptr->obj = object;
                kacpi_defwhile *dwhile = (kacpi_defwhile *)object;
                dwhile->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                distance = (uint64_t)aml_ptr;
                dwhile->pkglength = kacpi_getpkglength();
                aml_ptr++;
                dwhile->predicate = kacpi_gettermarg((uint64_t *)listptr);
                aml_ptr++;
                distance = (uint64_t)aml_ptr - distance;
                kacpi_gettermlist(dwhile->pkglength - distance, (uint64_t *)listptr, &dwhile->termlist);
                break;
            case 0xA4:
                kdebug_outf(" DefReturn");
                object = kmem_kalloc(sizeof(kacpi_defreturnop));
                listptr->obj = object;
                kacpi_defreturnop *dreturn = (kacpi_defreturnop *)object;
                dreturn->encodingvalue[0] = *aml_ptr;
                aml_ptr++;
                dreturn->argobject = kacpi_gettermarg((uint64_t *)listptr);
                break;
            case 0xA5:
                object = kmem_kalloc(sizeof(kacpi_expression));
                listptr->obj = object;
                object->encodingvalue[0] = *aml_ptr;
                break;
            case 0x5B:
                switch (*(aml_ptr + 1))
                {
                    case 0x27:
                        kdebug_outf(" ReleaseOp");
                        object = kmem_kalloc(sizeof(kacpi_defreleaseop));
                        listptr->obj = object;
                        kacpi_defreleaseop *release = (kacpi_defreleaseop *)object;
                        release->encodingvalue[0] = *aml_ptr;
                        aml_ptr++;
                        release->encodingvalue[1] = *aml_ptr;
                        aml_ptr++;
                        release->mutexobject = kacpi_getsupername((uint64_t *)listptr);
                        break;
                    case 0x80:
                        kdebug_outf(" OpRegionOp");
                        object = kmem_kalloc(sizeof(kacpi_defopregion));
                        listptr->obj = object;
                        kacpi_defopregion *opregion = (kacpi_defopregion *)object;
                        opregion->encodingvalue[0] = *aml_ptr;
                        aml_ptr++;
                        opregion->encodingvalue[1] = *aml_ptr;
                        aml_ptr++;
                        opregion->namestring = kacpi_getnamestring();
                        aml_ptr++;
                        opregion->regionspace = kacpi_getbytedata();
                        aml_ptr++;
                        opregion->regionoffset = kacpi_gettermarg((uint64_t *)listptr);
                        aml_ptr++;
                        opregion->regionlen = kacpi_gettermarg((uint64_t *)listptr);
                        break;
                    case 0x83:
                        kdebug_outf(" ProcessorOp");
                        object = kmem_kalloc(sizeof(kacpi_defprocessor));
                        listptr->obj = object;
                        kacpi_defprocessor *processor = (kacpi_defprocessor *)object;
                        processor->encodingvalue[0] = *aml_ptr;
                        aml_ptr++;
                        processor->encodingvalue[1] = *aml_ptr;
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr;
                        processor->pkglength = kacpi_getpkglength();
                        aml_ptr++;
                        processor->namestring = kacpi_getnamestring();
                        aml_ptr++;
                        processor->procid = kacpi_getbytedata();
                        aml_ptr++;
                        uint32_t addr = kacpi_getbytedata();
                        aml_ptr++;
                        addr += kacpi_getbytedata() << 8;
                        aml_ptr++;
                        addr += kacpi_getbytedata() << 16;
                        aml_ptr++;
                        addr += kacpi_getbytedata() << 24;
                        processor->pblkaddr = addr;
                        aml_ptr++;
                        processor->pblklen = kacpi_getbytedata();
                        aml_ptr++;
                        distance = (uint64_t)aml_ptr - distance;
                        kacpi_gettermlist(processor->pkglength - distance, (uint64_t *)listptr, &processor->termlist);
                        break;
                }
                if ((uint64_t)object)
                    break;
            default:
                object = kacpi_getexpression((uint64_t *)listptr);
                listptr->obj = object;
                if ((uint64_t)object == 0)
                    cont = 0;
                break;
        }
        if (cont)
        {
            listptr->next = kmem_kalloc(sizeof(kacpi_termlist));
            listptr = (kacpi_termlist *)listptr->next;
        }
        kacpi_depth--;
        if ((uint64_t)aml_ptr < end)
            aml_ptr++;
        else
            break;
    }
}

kacpi_termarg* kacpi_gettermarg(uint64_t *parent)
{
    kacpi_termarg *termarg = 0;
    uint64_t *ptr = 0;
    uint32_t len;

    kdebug_outf(" TA:%2x", *aml_ptr);

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
                if (*(aml_ptr + 1) == 0x30)
                {
                    termarg = kmem_kalloc(2);
                    termarg->type = *aml_ptr;
                    aml_ptr++;
                    termarg->data[0] = *aml_ptr;
                    break;
                }
            default:
                termarg = kmem_kalloc(9);
                ptr = (uint64_t *)termarg->data;
                kacpi_expression *expression = kacpi_getexpression(parent);
                *ptr = (uint64_t)expression;
                termarg->type = 0xF0;
                break;
        }
    }

    return termarg;
}

kacpi_datarefobj* kacpi_getdatarefobj()
{
    kacpi_datarefobj *datarefobj = 0;

    uint64_t distance = 0;
    uint64_t *ptr = 0;
    uint32_t *dwordptr = 0;
    uint16_t *wordptr = 0;

    switch (*aml_ptr)
    {
        case 0:
        case 1:
            datarefobj = kmem_kalloc(1);
            datarefobj->type = *aml_ptr;
            break;
        case 0x0A:
            datarefobj = kmem_kalloc(2);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            datarefobj->data[0] = kacpi_getbytedata();
            break;
        case 0x0B:
            datarefobj = kmem_kalloc(3);
            datarefobj->type = *aml_ptr;
            wordptr = (uint16_t *)&datarefobj->data;
            aml_ptr++;
            uint16_t word = kacpi_getbytedata();
            aml_ptr++;
            word += kacpi_getbytedata() << 8;
            *wordptr = word;
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
        case 0x0D:
            datarefobj = kmem_kalloc(1);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            size_t index = 0;
            while (*aml_ptr != 0)
            {
                kmem_kalloc(1);
                datarefobj->data[index] = *aml_ptr;
                aml_ptr++;
                index++;
            }
            kmem_kalloc(1);
            datarefobj->data[index] = 0;
            break;
        case 0x11:
            datarefobj = kmem_kalloc(9);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            ptr = (uint64_t *)datarefobj->data;
            kacpi_buffer *buffer = kmem_kalloc(24);
            *ptr = (uint64_t)buffer;
            buffer->pkglength = kacpi_getpkglength();
            aml_ptr++;
            buffer->buffersize = kacpi_gettermarg(0);
            aml_ptr++;
            uint32_t len = kacpi_termarginteger(buffer->buffersize);
            uint8_t* buf = kmem_kalloc(len);
            memcpy(buf, aml_ptr, len);
            buffer->bytelist = buf;
            aml_ptr += len - 1;
            break;
        case 0x12:
            datarefobj = kmem_kalloc(9);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            ptr = (uint64_t *)datarefobj->data;
            kacpi_defpackageop *package = kmem_kalloc(24);
            *ptr = (uint64_t)package;
            distance = (uint64_t)aml_ptr;
            package->pkglength = kacpi_getpkglength();
            aml_ptr++;
            package->varnumelements = kacpi_getbytedata();
            aml_ptr++;
            distance = (uint64_t)aml_ptr - distance;
            package->packageelementlist = kacpi_getpackageelementlist(
                package->pkglength - distance, package->varnumelements);
            break;
        case 0x13:
            datarefobj = kmem_kalloc(9);
            datarefobj->type = *aml_ptr;
            aml_ptr++;
            ptr = (uint64_t *)datarefobj->data;
            kacpi_defvarpackageop *varpackage = kmem_kalloc(24);
            *ptr = (uint64_t)varpackage;
            distance = (uint64_t)aml_ptr;
            varpackage->pkglength = kacpi_getpkglength();
            aml_ptr++;
            varpackage->varnumelements = kacpi_gettermarg(0);
            aml_ptr++;
            distance = (uint64_t)aml_ptr - distance;
            varpackage->packageelementlist = kacpi_getpackageelementlist(
                varpackage->pkglength - distance, kacpi_termarginteger(varpackage->varnumelements));
            break;
        default:
            kdebug_outf(" uhgdro %x", *aml_ptr);
            break;
    }

    return datarefobj;
}

uint64_t* kacpi_getpackageelementlist(uint32_t length, uint32_t elements)
{
    uint64_t *packageelementlist = 0;
    kacpi_packageelement *packageelement = 0;
    uint64_t end = (uint64_t)aml_ptr + length;
    uint32_t counted = 0;

    kdebug_outf("\nPEL: length%x", length);

    if (length > 0)
    {
        while (((uint64_t)aml_ptr < end) && (counted < elements))
        {
            if ((uint64_t)packageelement)
            {
                packageelement->next = kmem_kalloc(sizeof(kacpi_packageelement));
                packageelement = (kacpi_packageelement *)packageelement->next;
            }
            else
            {
                packageelement = kmem_kalloc(sizeof(kacpi_packageelement));
                packageelementlist = (uint64_t *)packageelement;
            }
            
            kacpi_datarefobj *datarefobj = kacpi_getdatarefobj();
            if ((uint64_t)datarefobj)
            {
                packageelement->type = 0;
                packageelement->value = (uint64_t)datarefobj;
                kdebug_outf(" datarefobj");
                aml_ptr++;
            }
            else
            {
                packageelement->type = 1;
                packageelement->value = (uint64_t)kacpi_getnamestring();
                kdebug_outf(" namestring");
                aml_ptr++;
            }

            counted++;
        }
    }

    if (elements - counted)
    {
        if ((uint64_t)packageelement)
        {
            packageelement->next = kmem_kalloc(sizeof(kacpi_packageelement));
            packageelement = (kacpi_packageelement *)packageelement->next;
        }
        else
        {
            packageelement = kmem_kalloc(sizeof(kacpi_packageelement));
            packageelementlist = (uint64_t *)packageelement;
        }
        packageelement->type = 2;
        packageelement->value = elements - counted;
    }
    aml_ptr = (uint8_t *)end - 1;
    kdebug_outf("\n");

    return packageelementlist;
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
    {
        names = 2;
        aml_ptr++;
    }
    else if (*aml_ptr == 0x2F)
    {
        aml_ptr++;
        names = *aml_ptr;
        aml_ptr++;
    }

    if (*aml_ptr == 0)
    {
        namepath = kmem_kalloc(1);
        namepath->names = 0;
        return namepath;
    }
    
    namepath = kmem_kalloc(1 + names * 4);
    namepath->names = names;

    memcpy(&namepath->name, aml_ptr, names * 4);
    aml_ptr += (names * 4) - 1;

    return namepath;
}

kacpi_namestring* kacpi_getnamestring()
{
    kacpi_namestring *namestring = kmem_kalloc(sizeof(kacpi_namestring));

    if (*aml_ptr == '\\')
    {
        namestring->first = '\\';
        aml_ptr++;
    }
    else if (*aml_ptr == '^')
        namestring->first = '^'; //change this to looping until prefix ends, allocate string into struct
    
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

void kacpi_printdepth()
{
    kdebug_outf("\n[%d]", depth);
    for (int i = 1; i < depth; i++)
        kdebug_outf("  ");
}

void kacpi_printnamestring(kacpi_namestring *namestring)
{
    if (namestring->namepath->names == 0)
        kdebug_outf(" nullname");
    else
    {
        kdebug_outf(" [%c", namestring->first);
        kdebug_outf("%4s", namestring->namepath->name[0]);
        for (int i = 1; i < namestring->namepath->names; i++)
            kdebug_outf(".%4s", namestring->namepath->name[i]);
        kdebug_outf("]");
    }
}

void kacpi_printsupername(kacpi_supername *supername)
{
    depth++;
    
    if (supername->type >= 0x60 && supername->type <= 0x67)
        kdebug_outf(" Local%dOp", supername->type - 0x60);
    else if (supername->type >= 0x68 && supername->type <= 0x6E)
        kdebug_outf(" Arg%dOp", supername->type - 0x68);
    else if (supername->type == 0)
        kdebug_outf(" Null");
    else
        kacpi_printtermarg((kacpi_termarg *)supername->data);
    
    depth--;
}

void kacpi_printtarget(kacpi_target *target)
{
    //kdebug_outf(" ->");
    //if (target->type == 0)
    //    kdebug_outf(" Null");
    //else
    if (target->type == 0)
        return;
    else
    {
        kdebug_outf(" ->");
        kacpi_supername *supername = (kacpi_supername *)target->data;
        kacpi_printsupername(supername);
    }
}

void kacpi_printtermarg(kacpi_termarg *termarg)
{
    //kdebug_outf(" [pta%x type%2x]", (uint64_t)termarg, termarg->type);
    if (termarg->type >= 0x60 && termarg->type <= 0x67)
        kdebug_outf("{Local%dOp}", termarg->type - 0x60);
    else if (termarg->type >= 0x68 && termarg->type <= 0x6E)
        kdebug_outf("{Arg%dOp}", termarg->type - 0x68);
    else
    {
        uint8_t type = termarg->type;
        uint8_t *data = termarg->data;
        kacpi_expression *expression = 0;
        if (type == 0xF0)
        {
            expression = (kacpi_expression *)*(uint64_t *)&termarg->data[0];
            type = expression->encodingvalue[0];
            data = expression->data;
            //kdebug_outf(" [type%2x expression%x]", type, (uint64_t)data);
        }
        kacpi_supername *supername = 0;
        switch (type)
        {
            case 0x0:
                kdebug_outf("{ZeroOp}");
                break;
            case 0x1:
                kdebug_outf("{OneOp}");
                break;
            case 0x0A:
            case 0x0B:
            case 0x0C:
                kdebug_outf("{0x%x}", kacpi_termarginteger(termarg));
                break;
            case 0x11:
                kacpi_buffer *buffer = (kacpi_buffer *)(expression);
                size_t size = kacpi_termarginteger(buffer->buffersize);
                kdebug_outf("Buffer, {");
                for (int i = 0; i < size; i++)
                    kdebug_outf(" %2x", buffer->bytelist[i]);
                kdebug_outf(" }");
                break;
            case 0x12:
                kacpi_defpackageop *package = (kacpi_defpackageop *)(expression);
                kdebug_outf("package [");
                depth++;
                kacpi_packageelement *packageelementlist = (kacpi_packageelement *)package->packageelementlist;
                while ((uint64_t)packageelementlist)
                {
                    kacpi_printdepth();
                    if (packageelementlist->type == 0)
                        kacpi_printdatarefobj((kacpi_datarefobj *)packageelementlist->value);
                    else if (packageelementlist->type == 1)
                        kacpi_printnamestring((kacpi_namestring *)packageelementlist->value);
                    else if (packageelementlist->type == 2)
                        kdebug_outf(" (%dx zeros)", (uint64_t)packageelementlist->value);
                    packageelementlist = (kacpi_packageelement *)packageelementlist->next;
                }
                depth--;
                kacpi_printdepth();
                kdebug_outf("]");
                break;
            case 0x72:
                kdebug_outf("(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" + ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                break;
            case 0x79:
                kdebug_outf("(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" << ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                break;
            case 0x7A:
                kdebug_outf("(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" >> ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                break;
            case 0x7B:
                kdebug_outf("(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" & ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                break;
            case 0x7D:
                kdebug_outf("(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" | ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                break;
            case 0x83:
                kdebug_outf(" DerefOf");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                break;
            case 0x86:
                kdebug_outf(" Notify(");
                supername = (kacpi_supername *)(*(uint64_t *)&data[0]);
                kacpi_printsupername(supername);
                kdebug_outf(", ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(")");
                break;
            case 0x87:
                kdebug_outf(" SizeOf(");
                supername = (kacpi_supername *)(*(uint64_t *)&data[0]);
                kacpi_printsupername(supername);
                kdebug_outf(")");
                break;
            case 0x88:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" [");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf("](");
                kacpi_printtarget((kacpi_target *)(*(uint64_t *)&data[16]));
                kdebug_outf(")");
                break;
            case 0x8A:
                kdebug_outf(" CreateDWordField(");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(",");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                kdebug_outf(",\'");
                kacpi_printnamestring((kacpi_namestring *)(*(uint64_t *)&data[16]));
                kdebug_outf("\')");
                break;
            case 0x90:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" && ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                break;
            case 0x91:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" || ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                break;
            case 0x92:
                if ((*(uint64_t *)&data[0] == 0x93))
                {
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                    kdebug_outf(" != ");
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[16]));
                }
                else if ((*(uint64_t *)&data[0] == 0x94))
                {
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                    kdebug_outf(" =< ");
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[16]));
                }
                else if ((*(uint64_t *)&data[0] == 0x95))
                {
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                    kdebug_outf(" >= ");
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[16]));
                }
                else
                {
                    kdebug_outf("!");
                    kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                }
                break;
            case 0x93:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" == ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                break;
            case 0x94:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" > ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                break;
            case 0x95:
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[0]));
                kdebug_outf(" < ");
                kacpi_printtermarg((kacpi_termarg *)(*(uint64_t *)&data[8]));
                break;
            case 0xFD:
                kdebug_outf("Field(");
                kacpi_printnamestring((kacpi_namestring *)(*(uint64_t *)&data[0]));
                kdebug_outf(")");
                break;
            case 0xFE:
                kacpi_defmethod *method = (kacpi_defmethod *)(*(uint64_t *)&data[0]);
                kdebug_outf(" Method(");
                kacpi_printnamestring(method->namestring);
                kdebug_outf(")");
                kacpi_termarglist *methodargs = (kacpi_termarglist *)(*(uint64_t *)&data[8]);
                for (uint8_t args = 0; args < (method->methodflags & 0b111); args++)
                {
                    kdebug_outf(" arg%d[", args);
                    kacpi_printtermarg(methodargs->arg);
                    kdebug_outf("]");
                    methodargs = (kacpi_termarglist *)methodargs->next;
                }
                break;
            case 0xFF:
                kdebug_outf(" OnesOp");
                break;
            default:
                kdebug_outf(" other %2x", type);
                break;
        }
    }
}

void kacpi_printfield(kacpi_fieldlist *fieldlist)
{
    kacpi_printdepth();
    kdebug_outf("field");
    depth++;
    kacpi_fieldlist *fieldlistptr = fieldlist;
    
    while ((uint64_t)fieldlistptr->element)
    {
        kacpi_fieldelement *element = fieldlistptr->element;
        kacpi_printdepth();
        switch (element->type)
        {
            case 0x00:
                kdebug_outf("resv len %x", ((uint32_t *)element->data)[0]);
                break;
            case 0x01:
                kdebug_outf("access type %x attrib %x", element->data[0], element->data[1]);
                break;
            case 0x02:
                kdebug_outf("connect field");
                break;
            case 0x03:
                kdebug_outf("ext access type %x ext attrib %x len 0x%x",
                    element->data[0], element->data[1], element->data[2]);
                break;
            default:
                kacpi_namepath *name = (kacpi_namepath *)*((uint64_t *)element->data);
                kdebug_outf("named %4s len %x", name->name[0], ((uint32_t *)element->data)[2]);
                break;
        }

        fieldlistptr = (kacpi_fieldlist *)fieldlistptr->next;
    }
    depth--;
}

void kacpi_printdatarefobj(kacpi_datarefobj *obj)
{
    kdebug_outf(" ref");
    switch (obj->type)
    {
        case 0:
            kdebug_outf(" [zero]");
            break;
        case 1:
            kdebug_outf(" [one]");
            break;
        case 0xA:
            kdebug_outf(" [%2x]", *obj->data);
            break;
        case 0xC:
            kdebug_outf(" [%8x]", *(uint32_t *)obj->data);
            break;
        case 0xD:
            kdebug_outf(" [\"%s\"]", (char *)obj->data);
            break;
        case 0x11:
            kacpi_buffer *buffer = (kacpi_buffer *)(*(uint64_t *)&obj->data);
            size_t size = kacpi_termarginteger(buffer->buffersize);
            kdebug_outf("Buffer, {");
            for (int i = 0; i < size; i++)
                kdebug_outf(" %2x", buffer->bytelist[i]);
            kdebug_outf(" }");
            break;
        case 0x12:
            kacpi_defpackageop *package = (kacpi_defpackageop *)(*(uint64_t *)&obj->data);
            kdebug_outf("package [");
            depth++;
            kacpi_packageelement *packageelementlist = (kacpi_packageelement *)package->packageelementlist;
            while ((uint64_t)packageelementlist)
            {
                kacpi_printdepth();
                if (packageelementlist->type == 0)
                    kacpi_printdatarefobj((kacpi_datarefobj *)packageelementlist->value);
                else if (packageelementlist->type == 1)
                    kacpi_printnamestring((kacpi_namestring *)packageelementlist->value);
                else if (packageelementlist->type == 2)
                    kdebug_outf(" (%dx zeros)", (uint64_t)packageelementlist->value);
                packageelementlist = (kacpi_packageelement *)packageelementlist->next;
            }
            depth--;
            kacpi_printdepth();
            kdebug_outf("]");
            break;
        default:
            kdebug_outf(" unknown dro %x", obj->type);
            break;
    }
}

void kacpi_printtermlistentry(kacpi_expression *obj)
{
    kacpi_defmethod *method = 0;
    switch (obj->encodingvalue[0])
    {
        case 0x08:
            kdebug_outf("name");
            depth++;
            kacpi_defname *name = (kacpi_defname *)obj;
            kacpi_printnamestring(name->namestring);
            kacpi_printdatarefobj(name->datarefobj);
            break;
        case 0x10:
            kdebug_outf("scope");
            depth++;
            kacpi_defscope *scope = (kacpi_defscope *)obj;
            kacpi_printnamestring(scope->namestring);
            kacpi_printtermlist(scope->termlist);
            break;
        case 0x13:
            kacpi_defvarpackageop *varpackage = (kacpi_defvarpackageop *)obj;
            kdebug_outf("varpackage pkglength %x", varpackage->pkglength);
            depth++;
            kacpi_printtermarg(varpackage->varnumelements);
            kacpi_packageelement *packageelementlist = (kacpi_packageelement *)varpackage->packageelementlist;
            while ((uint64_t)packageelementlist)
            {
                kacpi_printdepth();
                if (packageelementlist->type == 0)
                    kacpi_printdatarefobj((kacpi_datarefobj *)packageelementlist->value);
                else
                    kacpi_printnamestring((kacpi_namestring *)packageelementlist->value);
                packageelementlist = (kacpi_packageelement *)packageelementlist->next;
            }
            depth--;
            break;
        case 0x14:
            method = (kacpi_defmethod *)obj;
            kdebug_outf("method");
            depth++;
            kacpi_printnamestring(method->namestring);
            kdebug_outf(" flags %x", method->methodflags);
            kacpi_printtermlist(method->termlist);
            break;
        case 0x5B:
            switch (obj->encodingvalue[1])
            {
                case 0x01:
                    kacpi_defmutexop *mutex = (kacpi_defmutexop *)obj;
                    depth++;
                    kdebug_outf("mutex");
                    kacpi_printnamestring(mutex->namestring);
                    kdebug_outf(" sync flags %x", mutex->syncflags);
                    break;
                case 0x23:
                    kacpi_defacquireop *acquire = (kacpi_defacquireop *)obj;
                    depth++;
                    kdebug_outf("acquire");
                    kacpi_printsupername(acquire->mutexobject);
                    kdebug_outf("timeout:0x%x", acquire->timeout);
                    break;
                case 0x27:
                    kacpi_defreleaseop *release = (kacpi_defreleaseop *)obj;
                    depth++;
                    kdebug_outf("release");
                    kacpi_printsupername(release->mutexobject);
                    break;
                case 0x80:
                    kacpi_defopregion *opregion = (kacpi_defopregion *)obj;
                    depth++;
                    kdebug_outf("opregion");
                    kacpi_printnamestring(opregion->namestring);
                    kdebug_outf(" regionspace %x", opregion->regionspace);
                    kacpi_printtermarg(opregion->regionoffset);
                    kacpi_printtermarg(opregion->regionlen);
                    break;
                case 0x81:
                    kacpi_deffield *field = (kacpi_deffield *)obj;
                    kdebug_outf("field");
                    kacpi_printnamestring(field->namestring);
                    kdebug_outf(" flags %x", field->fieldflags);
                    depth++;
                    kacpi_printfield(field->fieldlist);
                    break;
                case 0x82:
                    kacpi_defdevice *device = (kacpi_defdevice *)obj;
                    kdebug_outf("device");
                    kacpi_printnamestring(device->namestring);
                    depth++;
                    kacpi_printtermlist(device->termlist);
                    break;
                case 0x83:
                    kacpi_defprocessor *processor = (kacpi_defprocessor *)obj;
                    kdebug_outf("processor");
                    kacpi_printnamestring(processor->namestring);
                    kdebug_outf(" id%2x addr%x len%2x", processor->procid,
                        processor->pblkaddr, processor->pblklen);
                    depth++;
                    kacpi_printtermlist(processor->termlist);
                    break;
                default:
                    kdebug_outf("unknown term %x", obj->encodingvalue[1]);
                    break;
            }
            break;
        case 0x70:
            depth++;
            kacpi_defstore *store = (kacpi_defstore *)obj;
            kacpi_printtermarg(store->operand);
            kdebug_outf(" ->");
            kacpi_printsupername(store->supername);
            break;
        case 0x74:
            depth++;
            kacpi_defsubtract *subtract = (kacpi_defsubtract *)obj;
            kacpi_printtermarg(subtract->operand1);
            kdebug_outf(" -");
            kacpi_printtermarg(subtract->operand2);
            kacpi_printtarget(subtract->target);
            break;
        case 0x75:
            depth++;
            kacpi_defincrement *increment = (kacpi_defincrement *)obj;
            kacpi_printsupername(increment->supername);
            kdebug_outf("++");
            break;
        case 0x79:
            depth++;
            kacpi_defshiftleft *left = (kacpi_defshiftleft *)obj;
            kacpi_printtermarg(left->operand);
            kdebug_outf(" <<");
            kacpi_printtermarg(left->count);
            kacpi_printtarget(left->target);
            break;
        case 0x7A:
            depth++;
            kacpi_defshiftright *right = (kacpi_defshiftright *)obj;
            kacpi_printtermarg(right->operand);
            kdebug_outf(" >>");
            kacpi_printtermarg(right->count);
            kacpi_printtarget(right->target);
            break;
        case 0x7D:
            depth++;
            kacpi_deforop *or = (kacpi_deforop *)obj;
            kacpi_printtermarg(or->operand1);
            kacpi_printtermarg(or->operand2);
            kdebug_outf(" |=");
            kacpi_printtarget(or->target);
            break;
        case 0x83:
            depth++;
            kacpi_defderefofop *derefof = (kacpi_defderefofop *)obj;
            kdebug_outf("derefof");
            kacpi_printtermarg(derefof->objreference);
            break;
        case 0x86:
            depth++;
            kapci_defnotify *notify = (kapci_defnotify *)obj;
            kdebug_outf("notify(");
            kacpi_printsupername(notify->notifyobject);
            kdebug_outf(", ");
            kacpi_printtermarg(notify->notifyvalue);
            kdebug_outf(")");
            break;
        case 0x8A:
            depth++;
            kacpi_defcreatedwordfield *createdwordfield = (kacpi_defcreatedwordfield *)obj;
            kdebug_outf("createDWordfield");
            kacpi_printtermarg(createdwordfield->buffer);
            kacpi_printtermarg(createdwordfield->byteindex);
            kacpi_printnamestring(createdwordfield->name);
            break;
        case 0x95:
            depth++;
            kacpi_defllessop *lless = (kacpi_defllessop *)obj;
            kacpi_printtermarg(lless->operand1);
            kdebug_outf(" < ");
            kacpi_printtermarg(lless->operand2);
            break;
        case 0x96:
            kdebug_outf("tobuffer");
            depth++;
            kacpi_deftobuffer *tobuffer = (kacpi_deftobuffer *)obj;
            kacpi_printtermarg(tobuffer->operand);
            kacpi_printtarget(tobuffer->target);
            break;
        case 0x98:
            kdebug_outf("tohexstring");
            depth++;
            kacpi_deftohexstring *tohexstring = (kacpi_deftohexstring *)obj;
            kacpi_printtermarg(tohexstring->operand);
            kacpi_printtarget(tohexstring->target);
            break;
        case 0xA0:
            kdebug_outf("if(");
            depth++;
            kacpi_defifop *dif = (kacpi_defifop *)obj;
            kacpi_printtermarg(dif->predicate);
            kdebug_outf(")");
            kacpi_printtermlist(dif->termlist);
            break;
        case 0xA1:
            kdebug_outf("else");
            depth++;
            kacpi_defelseop *delse = (kacpi_defelseop *)obj;
            kacpi_printtermlist(delse->termlist);
            break;
        case 0xA2:
            kdebug_outf("while");
            depth++;
            kacpi_defwhile *dwhile = (kacpi_defwhile *)obj;
            kacpi_printtermarg(dwhile->predicate);
            kacpi_printtermlist(dwhile->termlist);
            break;
        case 0xA4:
            kdebug_outf("return");
            depth++;
            kacpi_defreturnop *dreturn = (kacpi_defreturnop *)obj;
            kacpi_printtermarg(dreturn->argobject);
            break;
        case 0xA5:
            kdebug_outf("break");
            depth++;
            break;
        case 0xFD:
            kdebug_outf("Field(");
            kacpi_printnamestring((kacpi_namestring *)(*(uint64_t *)&obj->data[0]));
            kdebug_outf(")");
            depth++;
            break;
        case 0xFE:
            method = (kacpi_defmethod *)(*(uint64_t *)&obj->data[0]);
            kdebug_outf("method(");
            kacpi_printnamestring(method->namestring);
            kdebug_outf(")");
            kacpi_termarglist *methodargs = (kacpi_termarglist *)(*(uint64_t *)&obj->data[8]);
            for (uint8_t args = 0; args < (method->methodflags & 0b111); args++)
            {
                kdebug_outf(" arg%d[", args);
                kacpi_printtermarg(methodargs->arg);
                kdebug_outf("]");
                methodargs = (kacpi_termarglist *)methodargs->next;
            }
            depth++;
            break;
        default:
            kdebug_outf("unknown term %x", obj->encodingvalue[0]);
            break;
    }
}

void kacpi_printtermlist(kacpi_termlist *list)
{
    if ((uint64_t)list == 0)
    {
        kacpi_printdepth();
        kdebug_outf("empty list");
        return;
    }
    
    kacpi_termlist *listptr = list;

    while ((uint64_t)listptr->obj)
    {
        depth = listptr->depth;
        kacpi_printdepth();
        
        kacpi_expression *obj = listptr->obj;
        kacpi_printtermlistentry(obj);
        
        depth--;
        listptr = (kacpi_termlist *)listptr->next;
    }
}

void kacpi_printscope(kacpi_defscope *defscope)
{
    kacpi_printdepth();
    kdebug_outf("defscope");
    kacpi_printnamestring(defscope->namestring);
    
    depth++;
    kacpi_printtermlist(defscope->termlist);
    depth--;
}

void kacpi_parseaml(uint8_t *aml, size_t length)
{
    aml_ptr = aml;
    kdebug_outf("\r\nkacpi: parsing aml, %x %x", (uintptr_t)aml_ptr, (uintptr_t)aml_ptr + 5);

    kacpi_gettermlist(length, 0, &kacpi_dsdt);
    kacpi_printtermlist(kacpi_dsdt);

    //create real tree of scopes and devices
    //not just a term list
    // - this will solve method finding issue
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