#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/aml.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

void kacpi_aml_printop(const aml_op *op);

size_t indent = 0;

void print_indent()
{
    kdebug_outf("\n");
    for (size_t i = 0; i < indent; i++)
        kdebug_outf(" ");
}

void kacpi_aml_printtermlist(aml_termlist *tl)
{
    aml_termlist *tl_ptr = tl;
    while (tl_ptr != NULL)
    {
        if (tl_ptr->term_obj == NULL)
            break;
        kacpi_aml_printop(tl_ptr->term_obj);
        tl_ptr = tl_ptr->next;
    }
}

void kacpi_aml_printtarget(aml_target *target)
{
    switch (target->target_type)
    {
        case AML_TARGET_NULL:
            kdebug_outf("(null)");
            break;
        case AML_TARGET_DEBUG:
            kdebug_outf("(debug)");
            break;
        case AML_TARGET_REFERENCE:
            kdebug_outf("ref(");
            kacpi_aml_printop(*(aml_op **)target->target_data);
            kdebug_outf(")");
            break;
        case AML_TARGET_NAME:
            switch (target->target_data[0])
            {
                case AML_OP_LOCAL0 ... AML_OP_LOCAL7:
                    kdebug_outf("LOCAL%d", target->target_data[0] - 0x60);
                    break;
                case AML_OP_ARG0 ... AML_OP_ARG6:
                    kdebug_outf("ARG%d", target->target_data[0] - 0x68);
                    break;
                default:
                    kdebug_outf("%s", ((char **)target->target_data)[0]);
                    break;
            }
            break;
    }
}

void kacpi_aml_printfield(aml_fieldlist *fieldlist)
{
    aml_fieldlist *fieldlist_ptr = fieldlist;
    while (fieldlist_ptr != NULL)
    {
        if (fieldlist_ptr->element == NULL)
            break;
        print_indent();
        kdebug_outf("list entry %d", fieldlist_ptr->element->fieldtype);
        if (fieldlist_ptr->element->fieldtype == 4)
            kdebug_outf(" [%s]", ((aml_fieldelement_default *)fieldlist_ptr->element)->name);
        fieldlist_ptr = fieldlist_ptr->next;
    }
}

void kacpi_aml_printpackage(aml_packagelist *packagelist)
{
    aml_packagelist *packagelist_ptr = packagelist;
    while (packagelist_ptr != NULL)
    {
        if (packagelist_ptr->element == NULL)
            break;
        print_indent();
        kdebug_outf("element ");
        if (packagelist_ptr->element->fieldtype == 0)
            kacpi_aml_printop(((aml_packageelement_op *)packagelist_ptr->element)->value);
        else if (packagelist_ptr->element->fieldtype == 1)
            kdebug_outf("[%s]", ((aml_packageelement_name *)packagelist_ptr->element)->name);
        packagelist_ptr = packagelist_ptr->next;
    }
}

void kacpi_aml_printop(const aml_op *op)
{
    if (op == NULL)
        return;
    indent++;
    switch (op->op_code[0])
    {
        case AML_OP_ZERO:
            kdebug_outf("zero");
            break;
        case AML_OP_ONE:
            kdebug_outf("one");
            break;
        case AML_OP_ONES:
            kdebug_outf("ones");
            break;
        case AML_OP_ALIAS:
            aml_alias *alias = (aml_alias *)op;
            print_indent();
            kdebug_outf("alias [%s] [%s]", alias->namestring1, alias->namestring2);
            break;
        case AML_OP_NAME:
            aml_name *name = (aml_name *)op;
            print_indent();
            kdebug_outf("name [%s]: [", name->namestring);
            kacpi_aml_printop(name->datarefobj);
            kdebug_outf("]");
            break;
        case AML_OP_BYTECONST:
            aml_byteconst *byteconst = (aml_byteconst *)op;
            kdebug_outf("0x%x", byteconst->byteconst);
            break;
        case AML_OP_WORDCONST:
            aml_wordconst *wordconst = (aml_wordconst *)op;
            kdebug_outf("0x%x", wordconst->wordconst);
            break;
        case AML_OP_DWORDCONST:
            aml_dwordconst *dwordconst = (aml_dwordconst *)op;
            kdebug_outf("0x%x", dwordconst->dwordconst);
            break;
        case AML_OP_STRING:
            aml_stringconst *stringconst = (aml_stringconst *)op;
            kdebug_outf("\"%s\"", stringconst->stringconst);
            break;
        case AML_OP_QWORDCONST:
            aml_qwordconst *qwordconst = (aml_qwordconst *)op;
            kdebug_outf("0x%x", qwordconst->qwordconst);
            break;
        case AML_OP_SCOPE:
            aml_scope *scope = (aml_scope *)op;
            print_indent();
            kdebug_outf("scope [%s]", scope->namestring);
            kacpi_aml_printtermlist(scope->termlist);
            break;
        case AML_OP_BUFFER:
            aml_buffer *buffer = (aml_buffer *)op;
            kdebug_outf("buffer sz[");
            kacpi_aml_printop(buffer->buffersize);
            kdebug_outf("]");
            break;
        case AML_OP_PACKAGE:
            aml_package *package = (aml_package *)op;
            kdebug_outf("package elements[0x%x]", package->numelements);
            kacpi_aml_printpackage(package->packageelementlist);
            break;
        case AML_OP_METHOD:
            aml_method *method = (aml_method *)op;
            print_indent();
            kdebug_outf("method [%s] flags[%x]", method->namestring, method->methodflags);
            kacpi_aml_printtermlist(method->termlist);
            break;
        case AML_OP_STORE:
            aml_store *store = (aml_store *)op;
            print_indent();
            kdebug_outf("store termarg[");
            kacpi_aml_printop(store->termarg);
            kdebug_outf("] T[");
            kacpi_aml_printtarget(store->supername);
            kdebug_outf("]");
            break;
        case AML_OP_ADD ... AML_OP_SUBTRACT:
        case AML_OP_MULTIPLY:
        case AML_OP_SHIFTLEFT ... AML_OP_XOR:
        case AML_OP_CONCATRES ... AML_OP_MOD:
        case AML_OP_INDEX:
        case AML_OP_TOSTR:
            aml_subtract *subtract = (aml_subtract *)op;
            if (subtract->target->target_type != AML_TARGET_NULL)
                print_indent();
            if (op->op_code[0] == AML_OP_ADD)
                kdebug_outf("add");
            else if (op->op_code[0] == AML_OP_CONCAT)
                kdebug_outf("concat");
            else if (op->op_code[0] == AML_OP_SUBTRACT)
                kdebug_outf("subtract");
            else if (op->op_code[0] == AML_OP_MULTIPLY)
                kdebug_outf("multiply");
            else if (op->op_code[0] == AML_OP_SHIFTLEFT)
                kdebug_outf("shift left");
            else if (op->op_code[0] == AML_OP_SHIFTRIGHT)
                kdebug_outf("shift right");
            else if (op->op_code[0] == AML_OP_AND)
                kdebug_outf("and");
            else if (op->op_code[0] == AML_OP_NAND)
                kdebug_outf("nand");
            else if (op->op_code[0] == AML_OP_OR)
                kdebug_outf("or");
            else if (op->op_code[0] == AML_OP_NOR)
                kdebug_outf("nor");
            else if (op->op_code[0] == AML_OP_XOR)
                kdebug_outf("xor");
            else if (op->op_code[0] == AML_OP_CONCATRES)
                kdebug_outf("concatres");
            else if (op->op_code[0] == AML_OP_MOD)
                kdebug_outf("mod");
            else if (op->op_code[0] == AML_OP_INDEX)
                kdebug_outf("index");
            else if (op->op_code[0] == AML_OP_TOSTR)
                kdebug_outf("tostr");
            kdebug_outf(" operand[");
            kacpi_aml_printop(subtract->operand1);
            kdebug_outf("] operand[");
            kacpi_aml_printop(subtract->operand2);
            kdebug_outf("]");
            if (subtract->target->target_type != AML_TARGET_NULL)
            {
                kdebug_outf(" T[");
                kacpi_aml_printtarget(subtract->target);
                kdebug_outf("]");
            }
            break;
        case AML_OP_REFOF:
        case AML_OP_INCREMENT:
        case AML_OP_DECREMENT:
        case AML_OP_SIZEOF:
        case AML_OP_OBJTYPE:
            aml_sizeof *amlsizeof = (aml_sizeof *)op;
            if (op->op_code[0] == AML_OP_REFOF)
                kdebug_outf("refof");
            else if (op->op_code[0] == AML_OP_INCREMENT)
            {
                print_indent();
                kdebug_outf("increment");
            }
            else if (op->op_code[0] == AML_OP_DECREMENT)
            {
                print_indent();
                kdebug_outf("decrement");
            }
            else if (op->op_code[0] == AML_OP_SIZEOF)
                kdebug_outf("sizeof");
            else if (op->op_code[0] == AML_OP_OBJTYPE)
                kdebug_outf("objtype");
            kdebug_outf(" T[");
            kacpi_aml_printtarget(amlsizeof->supername);
            kdebug_outf("]");
            break;
        case AML_OP_CREATEDWF ... AML_OP_CREATEBIF:
        case AML_OP_CREATEQWF:
            aml_createdwordfield *cdwf = (aml_createdwordfield *)op;
            print_indent();
            kdebug_outf("create ");
            if (op->op_code[0] == AML_OP_CREATEDWF)
                kdebug_outf("dword field");
            else if (op->op_code[0] == AML_OP_CREATEWF)
                kdebug_outf("word field");
            else if (op->op_code[0] == AML_OP_CREATEBYF)
                kdebug_outf("byte field");
            else if (op->op_code[0] == AML_OP_CREATEBIF)
                kdebug_outf("bit field");
            else
                kdebug_outf("qword field");
            kdebug_outf(" source[");
            kacpi_aml_printop(cdwf->sourcebuff);
            kdebug_outf("] index[");
            kacpi_aml_printop(cdwf->index);
            kdebug_outf("] name[%s]", cdwf->namestring);
            break;
        case AML_OP_LAND ... AML_OP_LOR:
        case AML_OP_LEQUAL ... AML_OP_LLESS:
            aml_land *land = (aml_land *)op;
            kdebug_outf("{");
            kacpi_aml_printop(land->operand1);
            if (op->op_code[0] == AML_OP_LAND)
                kdebug_outf(" & ");
            else if (op->op_code[0] == AML_OP_OR)
                kdebug_outf(" | ");
            else if (op->op_code[0] == AML_OP_LEQUAL)
                kdebug_outf(" == ");
            else if (op->op_code[0] == AML_OP_LGREATER)
                kdebug_outf(" > ");
            else if (op->op_code[0] == AML_OP_LLESS)
                kdebug_outf(" < ");
            kacpi_aml_printop(land->operand2);
            kdebug_outf("}");
            break;
        case AML_OP_LNOT:
            aml_lnot *lnot = (aml_lnot *)op;
            kdebug_outf("!");
            kacpi_aml_printop(lnot->operand);
            break;
        case AML_OP_TOBUFFER ... AML_OP_TOINT:
            aml_tohexstring *tohexstring = (aml_tohexstring *)op;
            print_indent();
            if (op->op_code[0] == AML_OP_TOBUFFER)
                kdebug_outf("tobuffer");
            else if (op->op_code[0] == AML_OP_TODECSTR)
                kdebug_outf("todecimalstring");
            else if (op->op_code[0] == AML_OP_TOHEXSTR)
                kdebug_outf("tohexstring");
            else if (op->op_code[0] == AML_OP_TOINT)
                kdebug_outf("tointeger");
            kdebug_outf(" operand[");
            kacpi_aml_printop(tohexstring->operand);
            kdebug_outf("] T[");
            kacpi_aml_printtarget(tohexstring->target);
            kdebug_outf("]");
            break;
        case AML_OP_DEREFOF:
        case AML_OP_RETURN:
            aml_derefof *derefof = (aml_derefof *)op;
            if (op->op_code[0] == AML_OP_DEREFOF)
                kdebug_outf("deref [");
            else if (op->op_code[0] == AML_OP_RETURN)
            {
                print_indent();
                kdebug_outf("return [");
            }
            kacpi_aml_printop(derefof->objreference);
            kdebug_outf("]");
            break;
        case AML_OP_IF:
            aml_if *dif = (aml_if *)op;
            print_indent();
            kdebug_outf("if [");
            kacpi_aml_printop(dif->predicate);
            kdebug_outf("] do");
            kacpi_aml_printtermlist(dif->termlist);
            if (dif->defelse != NULL)
                kacpi_aml_printop(dif->defelse);
            break;
        case AML_OP_ELSE:
            aml_else *delse = (aml_else *)op;
            print_indent();
            kdebug_outf("else do");
            kacpi_aml_printtermlist(delse->termlist);
            break;
        case AML_OP_WHILE:
            aml_while *dwhile = (aml_while *)op;
            print_indent();
            kdebug_outf("while [");
            kacpi_aml_printop(dwhile->predicate);
            kdebug_outf("] do");
            kacpi_aml_printtermlist(dwhile->termlist);
            break;
        case AML_OPEXT_PREFIX:
            switch (op->op_code[1])
            {
                case AML_OPEXT_MUTEX:
                    aml_mutex *mutex = (aml_mutex *)op;
                    print_indent();
                    kdebug_outf("mutex [%s] sync[%x]", mutex->namestring, mutex->syncflags);
                    break;
                case AML_OPEXT_CONDREFOF:
                    aml_condrefof *condrefof = (aml_condrefof *)op;
                    print_indent();
                    kdebug_outf("condrefof [");
                    kacpi_aml_printtarget(condrefof->supername);
                    kdebug_outf("] [");
                    kacpi_aml_printtarget(condrefof->target);
                    kdebug_outf("]");
                    break;
                case AML_OPEXT_ACQUIRE:
                    aml_acquire *acquire = (aml_acquire *)op;
                    print_indent();
                    kdebug_outf("acquire [");
                    kacpi_aml_printtarget(acquire->mutexobject);
                    kdebug_outf("] timeout[0x%x]", acquire->timeout);
                    break;
                case AML_OPEXT_RELEASE:
                    aml_release *release = (aml_release *)op;
                    print_indent();
                    kdebug_outf("release [");
                    kacpi_aml_printtarget(release->eventobject);
                    kdebug_outf("]");
                    break;
                case AML_OPEXT_OPREGION:
                    aml_opregion *opregion = (aml_opregion *)op;
                    print_indent();
                    kdebug_outf("opregion [%s] space[%x]", opregion->namestring, opregion->regionspace);
                    kdebug_outf(" regionoffset[");
                    kacpi_aml_printop(opregion->regionoffset);
                    kdebug_outf("] regionlen[");
                    kacpi_aml_printop(opregion->regionlen);
                    kdebug_outf("]");
                    break;
                case AML_OPEXT_DEVICE:
                case AML_OPEXT_THERM_ZL:
                    aml_device *device = (aml_device *)op;
                    print_indent();
                    if (op->op_code[1] == AML_OPEXT_DEVICE)
                        kdebug_outf("device");
                    else if (op->op_code[1] == AML_OPEXT_THERM_ZL)
                        kdebug_outf("thermal zone");
                    kdebug_outf(" name[%s]", device->namestring);
                    kacpi_aml_printtermlist(device->termlist);
                    break;
                case AML_OPEXT_FIELD:
                    aml_field *field = (aml_field *)op;
                    print_indent();
                    kdebug_outf("field name[%s] flags[%x]", field->namestring, field->fieldflags);
                    indent++;
                    kacpi_aml_printfield(field->fieldlist);
                    indent--;
                    break;
                case AML_OPEXT_INDEXFIELD:
                    aml_indexfield *indexfield = (aml_indexfield *)op;
                    print_indent();
                    kdebug_outf("indexfield name[%s][%s] flags[%x]", indexfield->namestring1,
                        indexfield->namestring2, indexfield->fieldflags);
                    indent++;
                    kacpi_aml_printfield(indexfield->fieldlist);
                    indent--;
                    break;
                default:
                    print_indent();
                    kdebug_outf("print unknown ext %x", op->op_code[1]);
                    break;
            }
            break;
        case AML_OP_LOCAL0 ... AML_OP_LOCAL7:
            kdebug_outf("LOCAL%d", op->op_code[0] - 0x60);
            break;
        case AML_OP_ARG0 ... AML_OP_ARG6:
            kdebug_outf("ARG%d", op->op_code[0] - 0x68);
            break;
        case 0xFE:
            aml_methodinvocation *mi = (aml_methodinvocation *)op;
            print_indent();
            if (mi->termlist)
                kdebug_outf("METHOD");
            else if (mi->method)
                kdebug_outf("FIELD");
            else
                kdebug_outf("FUTURE OBJ");
            kdebug_outf(" [%s]", mi->namestring);
            break;
        default:
            kdebug_outf("print unknown %x", op->op_code[0]);
            break;
    }
    indent--;
}

aml_termlist *kacpi_aml_root;

void kacpi_processdsdt(uint64_t dsdt_addr)
{
    kdebug_outf("\nkacpi: processing dsdt from 0x%x", dsdt_addr);

    acpi_dsdt *dsdt = (acpi_dsdt *)virt_from_phys(dsdt_addr);

    // TODO IN THIS SECTION
    //  - Enumerate devices provided by ACPI
    //  - Enable ACPI mode and install events/interrupts

    uint8_t *aml = (uint8_t *)((uintptr_t)dsdt->aml);

    kacpi_aml_root = kmem_kalloc(sizeof(aml_termlist));
    kacpi_aml_generatetree(aml, dsdt->h.length - sizeof(acpi_sdt_header), kacpi_aml_root);

    kdebug_outf("\n\nkacpi: some aml hex codes - ");
    kacpi_aml_printtermlist(kacpi_aml_root);
    kdebug_outf("\n");
}