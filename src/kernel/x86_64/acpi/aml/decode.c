#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/aml.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

uint8_t *aml = 0;

char *nullstring = "\0";

aml_op *kacpi_aml_processop();

uint64_t kacpi_aml_intfromop(aml_op *op)
{
    switch (op->op_code[0])
    {
        case AML_OP_ZERO:
            return 0;
        case AML_OP_ONE:
            return 1;
        case AML_OP_ONES:
            return 1;
        case AML_OP_BYTECONST:
            aml_byteconst *byteconst = (aml_byteconst *)op;
            return byteconst->byteconst;
        case AML_OP_WORDCONST:
            aml_wordconst *wordconst = (aml_wordconst *)op;
            return wordconst->wordconst;
        case AML_OP_DWORDCONST:
            aml_dwordconst *dwordconst = (aml_dwordconst *)op;
            return dwordconst->dwordconst;
        case AML_OP_QWORDCONST:
            aml_qwordconst *qwordconst = (aml_qwordconst *)op;
            return qwordconst->qwordconst;
        default:
            kdebug_outf("\nunable to parse op %x into int", op->op_code[0]);
            while (1);
            break;
    }
}

uint8_t kacpi_aml_getbyte()
{
    return *aml++;
}

uint16_t kacpi_aml_getword()
{
    uint16_t word = 0;
    word += kacpi_aml_getbyte();
    word += kacpi_aml_getbyte() << 8;
    return word;
}

uint32_t kacpi_aml_getdword()
{
    uint32_t dword = 0;
    dword += kacpi_aml_getword();
    dword += kacpi_aml_getword() << 16;
    return dword;
}

uint64_t kacpi_aml_getqword()
{
    uint64_t qword = 0;
    qword += kacpi_aml_getdword();
    qword += (uint64_t)kacpi_aml_getdword() << 32;
    return qword;
}

uint32_t kacpi_aml_pkglength()
{
    uint32_t length = 0;

    uint8_t leadbyte = *aml++;
    uint8_t bytedata = leadbyte >> 6;

    if (bytedata == 0)
        length = leadbyte & 0x3F;
    else
    {
        length = leadbyte & 0xF;
        for (int i = 0; i < bytedata; i++)
            length += (*aml++ << (4 + 8*i));
    }

    return length;
}

char *kacpi_aml_namepath()
{
    char *namepath = NULL;
    char first_char = *aml;
    int names = 1;

    if (first_char == AML_NAME_NULL)
    {
        aml++;
        return nullstring;
    }
    else if (first_char == AML_NAME_DUAL)
    {
        aml++;
        names = 2;
    }
    else if (first_char == AML_NAME_MULTI)
    {
        aml++;
        names = kacpi_aml_getbyte();
    }

    namepath = kmem_kalloc(names * 4 + 1);
    memcpy(namepath, aml, names * 4);
    aml += (names * 4);

    namepath[names * 4] = 0;

    return namepath;
}

char *kacpi_aml_namestring()
{
    char *namestring = NULL;
    char *namepath = NULL;
    char first_char = *aml;

    if (first_char == 0)
        return nullstring;
    else if (first_char == AML_ROOTCHAR)
    {
        aml++;
        namepath = kacpi_aml_namepath();
        namestring = kmem_kalloc(str_len(namepath) + 2);
        memcpy(namestring + 1, namepath, str_len(namepath));
        namestring[0] = AML_ROOTCHAR;
        kmem_kfree(namepath);
    }
    else if (first_char == AML_PREFIXCHAR)
    {
        aml++;
        int prefix_count = 1;
        while (*aml == AML_PREFIXCHAR)
            aml++, prefix_count++;
        namepath = kacpi_aml_namepath();
        namestring = kmem_kalloc(str_len(namepath) + prefix_count + 1);
        memcpy(namestring + prefix_count, namepath, str_len(namepath));
        memset(namestring, AML_PREFIXCHAR, prefix_count);
        kmem_kfree(namepath);
    }
    else
        return kacpi_aml_namepath();

    return namestring;
}

aml_termlist *parent_termlist = 0;

aml_target *kacpi_aml_target()
{
    aml_target *target = NULL;
    uint8_t first_char = *aml;

    switch (first_char)
    {
        case 0:
            target = kmem_kalloc(1);
            target->target_type = AML_TARGET_NULL;
            aml++;
            break;
        case AML_OP_LOCAL0 ... AML_OP_ARG6:
            target = kmem_kalloc(2);
            target->target_type = AML_TARGET_NAME;
            target->target_data[0] = first_char;
            aml++;
            break;
        case AML_OPEXT_PREFIX:
            target = kmem_kalloc(1);
            target->target_type = AML_TARGET_DEBUG;
            aml += 2;
            break;
        case AML_OP_REFOF:
        case AML_OP_DEREFOF:
        case AML_OP_INDEX:
        //case USERTERMOBJ?
            target = kmem_kalloc(9);
            target->target_type = AML_TARGET_REFERENCE;
            *(aml_op **)target->target_data = kacpi_aml_processop();
            break;
        default:
            target = kmem_kalloc(9);
            target->target_type = AML_TARGET_NAME;
            ((char **)target->target_data)[0] = kacpi_aml_namestring();
            break;
    }

    return target;
}

aml_packagelist *kacpi_aml_packagelist(uint64_t length)
{
    aml_packagelist *packagelist = kmem_kalloc(sizeof(aml_packagelist));
    if (length == 0)
        return packagelist;

    uint64_t end_list = length + (uint64_t)aml;
    aml_packagelist *packagelist_ptr = packagelist;

    while ((uint64_t)aml < end_list)
    {
        switch (*aml)
        {
            case AML_OP_BYTECONST ... AML_OP_QWORDCONST:
            case AML_OP_ZERO:
            case AML_OP_ONE:
            case AML_OP_ONES:
            case AML_OPEXT_PREFIX:
            case AML_OP_PACKAGE:
            case AML_OP_VARPACKAGE:
                packagelist_ptr->element = kmem_kalloc(9);
                packagelist_ptr->element->fieldtype = 0;
                ((aml_op **)packagelist_ptr->element->fielddata)[0] = kacpi_aml_processop();
                break;
            default:
                packagelist_ptr->element = kmem_kalloc(9);
                packagelist_ptr->element->fieldtype = 1;
                ((char **)packagelist_ptr->element->fielddata)[0] = kacpi_aml_namestring();
                break;
        }
        packagelist_ptr->next = kmem_kalloc(sizeof(aml_packagelist));
        packagelist_ptr = packagelist_ptr->next;
    }

    return packagelist;
}

aml_fieldlist *kacpi_aml_fieldlist(uint64_t length)
{
    aml_fieldlist *fieldlist = kmem_kalloc(sizeof(aml_fieldlist));
    if (length == 0)
        return fieldlist;

    uint64_t end_list = length + (uint64_t)aml;
    aml_fieldlist *fieldlist_ptr = fieldlist;

    while ((uint64_t)aml < end_list)
    {
        switch (*aml)
        {
            case AML_FIELD_RESERVED:
                fieldlist_ptr->element = kmem_kalloc(9);
                fieldlist_ptr->element->fieldtype = AML_FIELD_RESERVED;
                aml++;
                *(uint32_t *)fieldlist_ptr->element->fielddata = kacpi_aml_pkglength();
                break;
            case AML_FIELD_ACCESS:
                fieldlist_ptr->element = kmem_kalloc(3);
                fieldlist_ptr->element->fieldtype = AML_FIELD_ACCESS;
                aml++;
                fieldlist_ptr->element->fielddata[0] = kacpi_aml_getbyte();
                fieldlist_ptr->element->fielddata[1] = kacpi_aml_getbyte();
                break;
            case AML_FIELD_CONNECT:
                kdebug_outf("\nconnect field not done yet");
                while (1);
                break;
            case AML_FIELD_EXACCESS:
                kdebug_outf("\nexaccess field not done yet");
                while (1);
                break;
            default:
                fieldlist_ptr->element = kmem_kalloc(13);
                fieldlist_ptr->element->fieldtype = 4;
                ((char **)fieldlist_ptr->element->fielddata)[0] = kacpi_aml_namepath();
                ((uint32_t *)fieldlist_ptr->element->fielddata)[2] = kacpi_aml_pkglength();
                break;
        }
        fieldlist_ptr->next = kmem_kalloc(sizeof(aml_fieldlist));
        fieldlist_ptr = fieldlist_ptr->next;
    }

    return fieldlist;
}

aml_termlist *kacpi_aml_termlist(uint64_t length, char *name)
{
    aml_termlist *termlist = kmem_kalloc(sizeof(aml_termlist));
    if (length == 0)
        return termlist;

    uint64_t end_list = length + (uint64_t)aml;
    aml_termlist *termlist_ptr = termlist;

    aml_termlist *last_parent = parent_termlist;
    parent_termlist = termlist;

    while ((uint64_t)aml < end_list)
    {
        termlist_ptr->parent = last_parent;
        termlist_ptr->listname = name;
        aml_op *op = kacpi_aml_processop();
        termlist_ptr->term_obj = op;
        termlist_ptr->next = kmem_kalloc(sizeof(aml_termlist));
        termlist_ptr = termlist_ptr->next;
    }

    parent_termlist = last_parent;

    return termlist;
}

aml_op *kacpi_aml_processop()
{
    uint8_t op = kacpi_aml_getbyte();
    aml_op *current_op = 0;
    uint64_t distance = 0;

    switch (op)
    {
        case AML_OP_ALIAS:
            aml_alias *alias = kmem_kalloc(sizeof(aml_alias));
            current_op = (aml_op *)alias;
            alias->op_code[0] = op;
            alias->namestring1 = kacpi_aml_namestring();
            alias->namestring2 = kacpi_aml_namestring();
            break;
        case AML_OP_NAME:
            aml_name *name = kmem_kalloc(sizeof(aml_name));
            current_op = (aml_op *)name;
            name->op_code[0] = op;
            name->namestring = kacpi_aml_namestring();
            name->datarefobj = kacpi_aml_processop();
            break;
        case AML_OP_BYTECONST:
            aml_byteconst *byteconst = kmem_kalloc(sizeof(aml_byteconst));
            current_op = (aml_op *)byteconst;
            byteconst->op_code[0] = op;
            byteconst->byteconst = kacpi_aml_getbyte();
            break;
        case AML_OP_WORDCONST:
            aml_wordconst *wordconst = kmem_kalloc(sizeof(aml_wordconst));
            current_op = (aml_op *)wordconst;
            wordconst->op_code[0] = op;
            wordconst->wordconst = kacpi_aml_getword();
            break;
        case AML_OP_DWORDCONST:
            aml_dwordconst *dwordconst = kmem_kalloc(sizeof(aml_dwordconst));
            current_op = (aml_op *)dwordconst;
            dwordconst->op_code[0] = op;
            dwordconst->dwordconst = kacpi_aml_getdword();
            break;
        case AML_OP_STRING:
            aml_stringconst *stringconst = kmem_kalloc(sizeof(aml_stringconst));
            current_op = (aml_op *)stringconst;
            stringconst->op_code[0] = op;
            distance = str_len((char *)aml);
            stringconst->stringconst = kmem_kalloc(distance + 1);
            memcpy(stringconst->stringconst, aml, distance);
            aml += distance + 1;
            break;
        case AML_OP_QWORDCONST:
            aml_qwordconst *qwordconst = kmem_kalloc(sizeof(aml_qwordconst));
            current_op = (aml_op *)qwordconst;
            qwordconst->op_code[0] = op;
            qwordconst->qwordconst = kacpi_aml_getqword();
            break;
        case AML_OP_SCOPE:
            aml_scope *scope = kmem_kalloc(sizeof(aml_scope));
            current_op = (aml_op *)scope;
            scope->op_code[0] = op;
            distance = (uint64_t)aml;
            scope->pkglength = kacpi_aml_pkglength();
            scope->namestring = kacpi_aml_namestring();
            distance = (uint64_t)aml - distance;
            scope->termlist = kacpi_aml_termlist(scope->pkglength - distance, scope->namestring);
            break;
        case AML_OP_BUFFER:
            aml_buffer *buffer = kmem_kalloc(sizeof(aml_buffer));
            current_op = (aml_op *)buffer;
            buffer->op_code[0] = op;
            buffer->pkglength = kacpi_aml_pkglength();
            buffer->buffersize = kacpi_aml_processop();
            distance = kacpi_aml_intfromop(buffer->buffersize);
            buffer->bytelist = kmem_kalloc(distance);
            memcpy(buffer->bytelist, aml, distance);
            aml += distance;
            break;
        case AML_OP_PACKAGE:
            aml_package *package = kmem_kalloc(sizeof(aml_package));
            current_op = (aml_op *)package;
            package->op_code[0] = op;
            distance = (uint64_t)aml;
            package->pkglength = kacpi_aml_pkglength();
            package->numelements = kacpi_aml_getbyte();
            distance = (uint64_t)aml - distance;
            package->packageelementlist = kacpi_aml_packagelist(package->pkglength - distance);
            break;
        case AML_OP_VARPACKAGE:
            aml_varpackage *varpackage = kmem_kalloc(sizeof(aml_varpackage));
            current_op = (aml_op *)varpackage;
            varpackage->op_code[0] = op;
            distance = (uint64_t)aml;
            varpackage->pkglength = kacpi_aml_pkglength();
            varpackage->varnumelements = kacpi_aml_processop();
            distance = (uint64_t)aml - distance;
            varpackage->packageelementlist = kacpi_aml_packagelist(varpackage->pkglength - distance);
            break;
        case AML_OP_METHOD:
            aml_method *method = kmem_kalloc(sizeof(aml_method));
            current_op = (aml_op *)method;
            method->op_code[0] = op;
            distance = (uint64_t)aml;
            method->pkglength = kacpi_aml_pkglength();
            method->namestring = kacpi_aml_namestring();
            method->methodflags = kacpi_aml_getbyte();
            aml = (uint8_t *)(distance + method->pkglength);
            //method->termlist = kacpi_aml_termlist(method->pkglength - distance, method->namestring);
            break;
        case AML_OP_EXTERNAL:
            aml_external *external = kmem_kalloc(sizeof(aml_external));
            current_op = (aml_op *)external;
            external->op_code[0] = op;
            external->namestring = kacpi_aml_namestring();
            external->objecttype = kacpi_aml_getbyte();
            external->argumentcount = kacpi_aml_getbyte();
            break;
        case AML_OP_STORE:
            aml_store *store = kmem_kalloc(sizeof(aml_store));
            current_op = (aml_op *)store;
            store->op_code[0] = op;
            store->termarg = kacpi_aml_processop();
            store->supername = kacpi_aml_target();
            break;
        case AML_OP_REFOF:
        case AML_OP_INCREMENT:
        case AML_OP_DECREMENT:
        case AML_OP_SIZEOF:
        case AML_OP_OBJTYPE:
            aml_refof *refof = kmem_kalloc(sizeof(aml_refof));
            current_op = (aml_op *)refof;
            refof->op_code[0] = op;
            refof->supername = kacpi_aml_target();
            break;
        case AML_OP_ADD ... AML_OP_SUBTRACT:
        case AML_OP_MULTIPLY:
        case AML_OP_SHIFTLEFT ... AML_OP_XOR:
        case AML_OP_CONCATRES ... AML_OP_MOD:
        case AML_OP_INDEX:
        case AML_OP_TOSTR:
            aml_add *add = kmem_kalloc(sizeof(aml_add));
            current_op = (aml_op *)add;
            add->op_code[0] = op;
            add->operand1 = kacpi_aml_processop();
            add->operand2 = kacpi_aml_processop();
            add->target = kacpi_aml_target();
            break;
        case AML_OP_DIVIDE:
            aml_divide *divide = kmem_kalloc(sizeof(aml_divide));
            current_op = (aml_op *)divide;
            divide->op_code[0] = op;
            divide->dividend = kacpi_aml_processop();
            divide->divisor = kacpi_aml_processop();
            divide->remainder = kacpi_aml_target();
            divide->quotient = kacpi_aml_target();
            break;
        case AML_OP_NOT ... AML_OP_FINDSRBIT:
        case AML_OP_TOBUFFER ... AML_OP_TOINT:
            aml_tohexstring *tohexstring = kmem_kalloc(sizeof(aml_tohexstring));
            current_op = (aml_op *)tohexstring;
            tohexstring->op_code[0] = op;
            tohexstring->operand = kacpi_aml_processop();
            tohexstring->target = kacpi_aml_target();
            break;
        case AML_OP_DEREFOF:
        case AML_OP_RETURN:
            aml_derefof *derefof = kmem_kalloc(sizeof(aml_derefof));
            current_op = (aml_op *)derefof;
            derefof->op_code[0] = op;
            derefof->objreference = kacpi_aml_processop();
            break;
        case AML_OP_NOTIFY:
            aml_notify *notify = kmem_kalloc(sizeof(aml_notify));
            current_op = (aml_op *)notify;
            notify->op_code[0] = op;
            notify->notifyobject = kacpi_aml_target();
            notify->notifyvalue = kacpi_aml_processop();
            break;
        case AML_OP_MATCH:
            aml_match *match = kmem_kalloc(sizeof(aml_match));
            current_op = (aml_op *)match;
            match->op_code[0] = op;
            match->searchpkg = kacpi_aml_processop();
            match->matchopcode1 = kacpi_aml_getbyte();
            match->operand1 = kacpi_aml_processop();
            match->matchopcode2 = kacpi_aml_getbyte();
            match->operand2 = kacpi_aml_processop();
            match->startindex = kacpi_aml_processop();
            break;
        case AML_OP_CREATEDWF ... AML_OP_CREATEBIF:
        case AML_OP_CREATEQWF:
            aml_createdwordfield *createdwf = kmem_kalloc(sizeof(aml_createdwordfield));
            current_op = (aml_op *)createdwf;
            createdwf->op_code[0] = op;
            createdwf->sourcebuff = kacpi_aml_processop();
            createdwf->index = kacpi_aml_processop();
            createdwf->namestring = kacpi_aml_namestring();
            break;
        case AML_OP_LAND ... AML_OP_LOR:
        case AML_OP_LEQUAL ... AML_OP_LLESS:
            aml_land *land = kmem_kalloc(sizeof(aml_land));
            current_op = (aml_op *)land;
            land->op_code[0] = op;
            land->operand1 = kacpi_aml_processop();
            land->operand2 = kacpi_aml_processop();
            break;
        case AML_OP_LNOT:
            aml_lnot *lnot = kmem_kalloc(sizeof(aml_lnot));
            current_op = (aml_op *)lnot;
            lnot->op_code[0] = op;
            lnot->operand = kacpi_aml_processop();
            break;
        case AML_OP_COPYOBJECT:
            aml_copyobject *copyobject = kmem_kalloc(sizeof(aml_copyobject));
            current_op = (aml_op *)copyobject;
            copyobject->op_code[0] = op;
            copyobject->operand = kacpi_aml_processop();
            copyobject->simplename = kacpi_aml_target();
            break;
        case AML_OP_MID:
            aml_mid *mid = kmem_kalloc(sizeof(aml_mid));
            current_op = (aml_op *)mid;
            mid->op_code[0] = op;
            mid->operand1 = kacpi_aml_processop();
            mid->operand2 = kacpi_aml_processop();
            mid->operand3 = kacpi_aml_processop();
            mid->target = kacpi_aml_target();
            break;
        case AML_OP_IF:
            aml_if *dif = kmem_kalloc(sizeof(aml_if));
            current_op = (aml_op *)dif;
            dif->op_code[0] = op;
            distance = (uint64_t)aml;
            dif->pkglength = kacpi_aml_pkglength();
            dif->predicate = kacpi_aml_processop();
            distance = (uint64_t)aml - distance;
            dif->termlist = kacpi_aml_termlist(dif->pkglength - distance, 0);
            if (*aml == AML_OP_ELSE)
                dif->defelse = kacpi_aml_processop();
            break;
        case AML_OP_ELSE:
            aml_else *delse = kmem_kalloc(sizeof(aml_else));
            current_op = (aml_op *)delse;
            delse->op_code[0] = op;
            distance = (uint64_t)aml;
            delse->pkglength = kacpi_aml_pkglength();
            distance = (uint64_t)aml - distance;
            delse->termlist = kacpi_aml_termlist(delse->pkglength - distance, 0);
            break;
        case AML_OP_WHILE:
            aml_while *dwhile = kmem_kalloc(sizeof(aml_while));
            current_op = (aml_op *)dwhile;
            dwhile->op_code[0] = op;
            distance = (uint64_t)aml;
            dwhile->pkglength = kacpi_aml_pkglength();
            dwhile->predicate = kacpi_aml_processop();
            distance = (uint64_t)aml - distance;
            dwhile->termlist = kacpi_aml_termlist(dwhile->pkglength - distance, 0);
            break;
        case AML_OPEXT_PREFIX:
            uint8_t extop = kacpi_aml_getbyte();
            switch (extop)
            {
                case AML_OPEXT_MUTEX:
                    aml_mutex *mutex = kmem_kalloc(sizeof(aml_mutex));
                    current_op = (aml_op *)mutex;
                    mutex->op_code[0] = AML_OPEXT_PREFIX;
                    mutex->op_code[1] = extop;
                    mutex->namestring = kacpi_aml_namestring();
                    mutex->syncflags = kacpi_aml_getbyte();
                    break;
                case AML_OPEXT_EVENT:
                    aml_event *event = kmem_kalloc(sizeof(aml_event));
                    current_op = (aml_op *)event;
                    event->op_code[0] = AML_OPEXT_PREFIX;
                    event->op_code[1] = extop;
                    event->namestring = kacpi_aml_namestring();
                    break;
                case AML_OPEXT_CONDREFOF:
                    aml_condrefof *condrefof = kmem_kalloc(sizeof(aml_condrefof));
                    current_op = (aml_op *)condrefof;
                    condrefof->op_code[0] = AML_OPEXT_PREFIX;
                    condrefof->op_code[1] = extop;
                    condrefof->supername = kacpi_aml_target();
                    condrefof->target = kacpi_aml_target();
                    break;
                case AML_OPEXT_CREATEFIELD:
                    aml_createfield *createfield = kmem_kalloc(sizeof(aml_createfield));
                    current_op = (aml_op *)createfield;
                    createfield->op_code[0] = AML_OPEXT_PREFIX;
                    createfield->op_code[1] = extop;
                    createfield->sourcebuff = kacpi_aml_processop();
                    createfield->bitindex = kacpi_aml_processop();
                    createfield->numbits = kacpi_aml_processop();
                    createfield->namestring = kacpi_aml_namestring();
                    break;
                case AML_OPEXT_LOADTABLE:
                    aml_loadtable *loadtable = kmem_kalloc(sizeof(aml_loadtable));
                    current_op = (aml_op *)loadtable;
                    loadtable->op_code[0] = AML_OPEXT_PREFIX;
                    loadtable->op_code[1] = extop;
                    loadtable->term1 = kacpi_aml_processop();
                    loadtable->term2 = kacpi_aml_processop();
                    loadtable->term3 = kacpi_aml_processop();
                    loadtable->term4 = kacpi_aml_processop();
                    loadtable->term5 = kacpi_aml_processop();
                    loadtable->term6 = kacpi_aml_processop();
                    break;
                case AML_OPEXT_LOAD:
                    aml_load *load = kmem_kalloc(sizeof(aml_load));
                    current_op = (aml_op *)load;
                    load->op_code[0] = AML_OPEXT_PREFIX;
                    load->op_code[1] = extop;
                    load->namestring = kacpi_aml_namestring();
                    load->target = kacpi_aml_target();
                    break;
                case AML_OPEXT_STALL ... AML_OPEXT_SLEEP:
                    aml_stall *stall = kmem_kalloc(sizeof(aml_stall));
                    current_op = (aml_op *)stall;
                    stall->op_code[0] = AML_OPEXT_PREFIX;
                    stall->op_code[1] = extop;
                    stall->time = kacpi_aml_processop();
                    break;
                case AML_OPEXT_ACQUIRE:
                    aml_acquire *acquire = kmem_kalloc(sizeof(aml_acquire));
                    current_op = (aml_op *)acquire;
                    acquire->op_code[0] = AML_OPEXT_PREFIX;
                    acquire->op_code[1] = extop;
                    acquire->mutexobject = kacpi_aml_target();
                    acquire->timeout = kacpi_aml_getword();
                    break;
                case AML_OPEXT_SIGNAL:
                case AML_OPEXT_RESET:
                case AML_OPEXT_RELEASE:
                    aml_signal *signal = kmem_kalloc(sizeof(aml_signal));
                    current_op = (aml_op *)signal;
                    signal->op_code[0] = AML_OPEXT_PREFIX;
                    signal->op_code[1] = extop;
                    signal->eventobject = kacpi_aml_target();
                    break;
                case AML_OPEXT_WAIT:
                    aml_wait *wait = kmem_kalloc(sizeof(aml_wait));
                    current_op = (aml_op *)wait;
                    wait->op_code[0] = AML_OPEXT_PREFIX;
                    wait->op_code[1] = extop;
                    wait->eventobject = kacpi_aml_target();
                    wait->operand = kacpi_aml_processop();
                    break;
                case AML_OPEXT_FROMBCD:
                case AML_OPEXT_TOBCD:
                    aml_frombcd *frombcd = kmem_kalloc(sizeof(aml_frombcd));
                    current_op = (aml_op *)frombcd;
                    frombcd->op_code[0] = AML_OPEXT_PREFIX;
                    frombcd->op_code[1] = extop;
                    frombcd->operand = kacpi_aml_processop();
                    frombcd->target = kacpi_aml_target();
                    break;
                case AML_OPEXT_FATAL:
                    aml_fatal *fatal = kmem_kalloc(sizeof(aml_fatal));
                    current_op = (aml_op *)fatal;
                    fatal->op_code[0] = AML_OPEXT_PREFIX;
                    fatal->op_code[1] = extop;
                    fatal->fataltype = kacpi_aml_getbyte();
                    fatal->fatalcode = kacpi_aml_getdword();
                    fatal->fatalarg = kacpi_aml_processop();
                    break;
                case AML_OPEXT_OPREGION:
                    aml_opregion *opregion = kmem_kalloc(sizeof(aml_opregion));
                    current_op = (aml_op *)opregion;
                    opregion->op_code[0] = AML_OPEXT_PREFIX;
                    opregion->op_code[1] = extop;
                    opregion->namestring = kacpi_aml_namestring();
                    opregion->regionspace = kacpi_aml_getbyte();
                    opregion->regionoffset = kacpi_aml_processop();
                    opregion->regionlen = kacpi_aml_processop();
                    break;
                case AML_OPEXT_FIELD:
                    aml_field *field = kmem_kalloc(sizeof(aml_field));
                    current_op = (aml_op *)field;
                    field->op_code[0] = AML_OPEXT_PREFIX;
                    field->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    field->pkglength = kacpi_aml_pkglength();
                    field->namestring = kacpi_aml_namestring();
                    field->fieldflags = kacpi_aml_getbyte();
                    distance = (uint64_t)aml - distance;
                    field->fieldlist = kacpi_aml_fieldlist(field->pkglength - distance);
                    break;
                case AML_OPEXT_DEVICE:
                case AML_OPEXT_THERM_ZL:
                    aml_device *device = kmem_kalloc(sizeof(aml_device));
                    current_op = (aml_op *)device;
                    device->op_code[0] = AML_OPEXT_PREFIX;
                    device->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    device->pkglength = kacpi_aml_pkglength();
                    device->namestring = kacpi_aml_namestring();
                    distance = (uint64_t)aml - distance;
                    device->termlist = kacpi_aml_termlist(device->pkglength - distance, device->namestring);
                    break;
                case AML_OPEXT_PROCESSOR:
                    aml_processor *processor = kmem_kalloc(sizeof(aml_processor));
                    current_op = (aml_op *)processor;
                    processor->op_code[0] = AML_OPEXT_PREFIX;
                    processor->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    processor->pkglength = kacpi_aml_pkglength();
                    processor->namestring = kacpi_aml_namestring();
                    processor->id = kacpi_aml_getbyte();
                    processor->pblkaddr = kacpi_aml_getdword();
                    processor->pblklen = kacpi_aml_getbyte();
                    distance = (uint64_t)aml - distance;
                    processor->termlist = kacpi_aml_termlist(processor->pkglength - distance, processor->namestring);
                    break;
                case AML_OPEXT_POWERRES:
                    aml_powerres *powerres = kmem_kalloc(sizeof(aml_powerres));
                    current_op = (aml_op *)powerres;
                    powerres->op_code[0] = AML_OPEXT_PREFIX;
                    powerres->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    powerres->pkglength = kacpi_aml_pkglength();
                    powerres->namestring = kacpi_aml_namestring();
                    powerres->systemlevel = kacpi_aml_getbyte();
                    powerres->resourceorder = kacpi_aml_getword();
                    distance = (uint64_t)aml - distance;
                    powerres->termlist = kacpi_aml_termlist(powerres->pkglength - distance, powerres->namestring);
                    break;
                case AML_OPEXT_INDEXFIELD:
                    aml_indexfield *indexfield = kmem_kalloc(sizeof(aml_indexfield));
                    current_op = (aml_op *)indexfield;
                    indexfield->op_code[0] = AML_OPEXT_PREFIX;
                    indexfield->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    indexfield->pkglength = kacpi_aml_pkglength();
                    indexfield->namestring1 = kacpi_aml_namestring();
                    indexfield->namestring2 = kacpi_aml_namestring();
                    indexfield->fieldflags = kacpi_aml_getbyte();
                    distance = (uint64_t)aml - distance;
                    indexfield->fieldlist = kacpi_aml_fieldlist(indexfield->pkglength - distance);
                    break;
                case AML_OPEXT_BANKFIELD:
                    aml_bankfield *bankfield = kmem_kalloc(sizeof(aml_bankfield));
                    current_op = (aml_op *)bankfield;
                    bankfield->op_code[0] = AML_OPEXT_PREFIX;
                    bankfield->op_code[1] = extop;
                    distance = (uint64_t)aml;
                    bankfield->namestring1 = kacpi_aml_namestring();
                    bankfield->namestring2 = kacpi_aml_namestring();
                    bankfield->bankvalue = kacpi_aml_processop();
                    bankfield->fieldflags = kacpi_aml_getbyte();
                    distance = (uint64_t)aml - distance;
                    bankfield->fieldlist = kacpi_aml_fieldlist(bankfield->pkglength - distance);
                    break;
                case AML_OPEXT_DATAREGION:
                    aml_dataregion *dataregion = kmem_kalloc(sizeof(aml_dataregion));
                    current_op = (aml_op *)dataregion;
                    dataregion->op_code[0] = AML_OPEXT_PREFIX;
                    dataregion->op_code[1] = extop;
                    dataregion->namestring = kacpi_aml_namestring();
                    dataregion->termarg1 = kacpi_aml_processop();
                    dataregion->termarg2 = kacpi_aml_processop();
                    dataregion->termarg3 = kacpi_aml_processop();
                    break;
                case AML_OPEXT_REVISION:
                case AML_OPEXT_DEBUG:
                case AML_OPEXT_TIMER:
                    current_op = kmem_kalloc(3);
                    current_op->op_code[0] = AML_OPEXT_PREFIX;
                    current_op->op_code[1] = extop;
                    break;
                default:
                    kdebug_outf("\nunknown opext code %x", op);
                    break;
            }
            break;
        case AML_OP_ZERO:
        case AML_OP_ONE:
        case AML_OP_ONES:
        case AML_OP_LOCAL0 ... AML_OP_ARG6:
        case AML_OP_CONTINUE:
        case AML_OP_NOOP:
        case AML_OP_BREAK:
        case AML_OP_BREAKPOINT:
            current_op = kmem_kalloc(3);
            current_op->op_code[0] = op;
            break;
        case 'A' ... 'Z':
        case '_':
            aml_methodinvocation *methodinvocation = kmem_kalloc(sizeof(aml_methodinvocation));
            current_op = (aml_op *)methodinvocation;
            methodinvocation->op_code[0] = 0xFE;
            aml--;
            methodinvocation->namestring = kacpi_aml_namestring();
            aml_op *object = 0;
            if (methodinvocation->namestring[0] != AML_ROOTCHAR)
            {
                aml_termlist *named_parent = parent_termlist;

                while (named_parent->listname == 0 && named_parent->parent)
                    named_parent = named_parent->parent;

                object = kacpi_aml_findtreename(named_parent, methodinvocation->namestring);
                
                while (object == NULL)
                {
                    if (named_parent->parent == 0)
                        break;
                    named_parent = named_parent->parent;

                    while (named_parent->listname == 0 && named_parent->parent)
                        named_parent = named_parent->parent;
                    
                    object = kacpi_aml_findtreename(named_parent, methodinvocation->namestring);
                }
            }
            else
                object = kacpi_aml_findtreename(kacpi_aml_root, methodinvocation->namestring);

            if (object == NULL)
                kdebug_outf("\nobject not found!");
            else
                kacpi_aml_printop(object);

            methodinvocation->method = object;

            if (methodinvocation->method->op_code[0] == AML_OP_METHOD)
            {
                methodinvocation->termlist = kmem_kalloc(sizeof(aml_termlist));
                uint8_t argcount = ((aml_method *)methodinvocation->method)->methodflags & 0b111;

                aml_termlist *method_arg = methodinvocation->termlist;

                for (uint8_t i = 1; i <= argcount; i++)
                {
                    method_arg->term_obj = kacpi_aml_processop();
                    if (i != argcount)
                    {
                        method_arg->next = kmem_kalloc(sizeof(aml_termlist));
                        method_arg = method_arg->next;
                    }
                }
            }
            else
                methodinvocation->termlist = 0;
            
            break;
        default:
            kdebug_outf("\nunknown op code %x", op);
            break;
    }

    return current_op;
}

void kacpi_aml_generatetree(uint8_t *aml_ptr, size_t length, aml_termlist *tree)
{
    aml_termlist *tree_ptr = tree;
    parent_termlist = tree;

    aml = aml_ptr;
    size_t aml_end = (size_t)aml + length;

    while ((size_t)aml < aml_end)
    {
        aml_op *op = kacpi_aml_processop();
        tree_ptr->term_obj = op;
        tree_ptr->parent = 0;
        tree_ptr->listname = "\\";
        tree_ptr->next = kmem_kalloc(sizeof(aml_termlist));
        tree_ptr = tree_ptr->next;
    }
}