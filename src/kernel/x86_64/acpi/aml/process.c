#include <x86_64/acpi/acpi.h>
#include <x86_64/acpi/aml.h>

#include <kernel/kstring.h>
#include <kernel/debug.h>

#include <mm/mem.h>

aml_termlist *get_termlist(aml_op *op)
{
    switch (op->op_code[0])
    {
        case AML_OP_SCOPE:
            return ((aml_scope *)op)->termlist;
        case AML_OP_METHOD:
            return ((aml_method *)op)->termlist;
        default:
            return ((aml_device *)op)->termlist;
    }
}

aml_op *kacpi_aml_findtreename(aml_termlist *start, char *givenname)
{
    aml_termlist *tree_ptr = start;
    char *name = givenname;

    if (name[0] == AML_PREFIXCHAR)
    {
        size_t prefix = 0;
        while (name[prefix] == AML_PREFIXCHAR)
            prefix++;

        name += prefix;

        while (tree_ptr->listname == 0)
            tree_ptr = tree_ptr->parent;
        while (prefix > 0 && tree_ptr)
        {
            if (tree_ptr->listname && tree_ptr->listname[0] != AML_PREFIXCHAR)
                prefix--;
            tree_ptr = tree_ptr->parent;
        }
    }

    char *startname = kacpi_aml_gettreename(tree_ptr);
    if (startname == 0)
        return NULL;

    //kdebug_outf("\nstartname [%s]", startname);

    char *fullname = 0;
    if (name[0] == AML_ROOTCHAR)
    {
        fullname = kmem_kalloc(str_len(name) + 1);
        memcpy(fullname, name, str_len(name));
    }
    else
    {
        fullname = kmem_kalloc(str_len(startname) + str_len(name) + 1);
        memcpy(fullname, startname, str_len(startname));
        memcpy(fullname + str_len(startname), name, str_len(name));
    }

    //kdebug_outf("\nlooking for [%s]", fullname);

    while (tree_ptr != NULL)
    {
        aml_op *obj = tree_ptr->term_obj;
        if (obj == NULL)
            break;

        //kdebug_outf("\nobj %2x%2x", obj->op_code[0], obj->op_code[1]);

        char *current_name = 0;

        if (obj->op_code[1] == AML_OPEXT_FIELD || obj->op_code[1] == AML_OPEXT_INDEXFIELD)
        {
            aml_fieldlist *fieldlist_ptr = 0;
            current_name = kacpi_aml_gettreename(tree_ptr);
            if (obj->op_code[1] == AML_OPEXT_FIELD)
                fieldlist_ptr = ((aml_field *)obj)->fieldlist;
            else if (obj->op_code[1] == AML_OPEXT_INDEXFIELD)
                fieldlist_ptr = ((aml_indexfield *)obj)->fieldlist;
            while (fieldlist_ptr != NULL)
            {
                if (fieldlist_ptr->element != NULL)
                {
                    if (fieldlist_ptr->element->fieldtype == 4)
                    {
                        char *fieldname = ((aml_fieldelement_default *)fieldlist_ptr->element)->name;
                        //kdebug_outf("\n[%s][%s] vs [%s]", current_name, fieldname, fullname);
                        if ((strn_cmp(current_name, fullname, str_len(current_name)) == 0) &&
                            (strn_cmp(fieldname, fullname + str_len(current_name), str_len(fieldname)) == 0))
                        {
                            kmem_kfree(fullname);
                            return obj;
                        }
                    }
                }
                fieldlist_ptr = fieldlist_ptr->next;
            }
        }
        else
        {
            char *checkname = 0;
            current_name = kacpi_aml_gettreename(tree_ptr);

            if (obj->op_code[0] == AML_OP_NAME)
                checkname = ((aml_name *)obj)->namestring;
            else if (obj->op_code[0] == AML_OP_METHOD)
                checkname = ((aml_method *)obj)->namestring;

            if (checkname)
            {
                //kdebug_outf("\n[%s][%s] vs [%s]", current_name, checkname, fullname);
                if ((strn_cmp(current_name, fullname, str_len(current_name)) == 0) &&
                    (strn_cmp(checkname, fullname + str_len(current_name), str_len(checkname)) == 0))
                {
                    kmem_kfree(fullname);
                    return obj;
                }
            }
        }
        

        if (obj->op_code[0] == AML_OP_SCOPE ||
            obj->op_code[0] == AML_OP_METHOD ||
            (obj->op_code[0] == AML_OPEXT_PREFIX && obj->op_code[1] == AML_OPEXT_DEVICE))
        {
            aml_termlist *list = get_termlist(obj);
            //kdebug_outf("\nlist %x treeptr %x", list, tree_ptr);
            if (list && (list != tree_ptr))
            {
                aml_op *check = kacpi_aml_findtreename(list, givenname);
                if (check)
                {
                    kmem_kfree(fullname);
                    return check;
                }
            }
        }
        
        tree_ptr = tree_ptr->next;
    }

    kmem_kfree(fullname);
    return NULL;
}

void kacpi_aml_setlistname(aml_termlist *list, char *name)
{
    aml_termlist *list_ptr = list->front;

    while (list_ptr)
    {
        list_ptr->fullname = name;
        list_ptr = list_ptr->next;
    }
}

char *kacpi_aml_gettreename(aml_termlist *tree)
{
    aml_termlist *tree_ptr = tree;
    char *tree_name = NULL;

    //kdebug_outf("\nget name [%x]", tree);

    aml_termlist *firstnamed = 0;

    while (tree_ptr)
    {
        if (tree_ptr->listname != 0 && tree_ptr->listname[0] != '\0')
        {
            if (firstnamed == 0)
            {
                firstnamed = tree_ptr;
                if (tree_ptr->fullname)
                    return tree_ptr->fullname;
            }

            if (tree_name == NULL)
            {
                tree_name = kmem_kalloc(str_len(tree_ptr->listname) + 1);
                memcpy(tree_name, tree_ptr->listname, str_len(tree_ptr->listname));
            }
            else
            {
                char *new = kmem_kalloc(str_len(tree_name) + str_len(tree_ptr->listname) + 1);
                memcpy(new + str_len(tree_ptr->listname), tree_name, str_len(tree_name));
                memcpy(new, tree_ptr->listname, str_len(tree_ptr->listname));
                kmem_kfree(tree_name);
                tree_name = new;
            }

            if (tree_name[0] == AML_PREFIXCHAR)
            {
                size_t prefix = 0;
                size_t prefix_len = 0;
                while (tree_name[prefix] == AML_PREFIXCHAR)
                    prefix++;

                char *name = tree_name + prefix;

                while (prefix > 0 && tree_ptr)
                {
                    if (tree_ptr->listname && tree_ptr->listname[0] != AML_PREFIXCHAR)
                    {
                        prefix--;
                        prefix_len += str_len(tree_ptr->listname);
                    }
                    tree_ptr = tree_ptr->parent;
                }

                char *prefix_name = kacpi_aml_gettreename(tree_ptr);

                char *new = kmem_kalloc(str_len(prefix_name) + str_len(name) + 1);
                memcpy(new, prefix_name, str_len(prefix_name));
                memcpy(new + str_len(prefix_name), name, str_len(name));
                kmem_kfree(tree_name);

                kacpi_aml_setlistname(firstnamed, new);

                return new;
            }
        }

        if (tree_name && tree_name[0] == AML_ROOTCHAR)
            break;
        tree_ptr = tree_ptr->parent;
    }

    if (tree_name != NULL)
        kacpi_aml_setlistname(firstnamed, tree_name);

    return tree_name;
}

aml_termarg *aml_local[8];
aml_termarg *aml_arg[7];
aml_termarg *aml_returnval;

aml_termarg *kacpi_aml_resolvevalue(aml_termarg *term)
{
    aml_termarg *newarg = NULL;

    if (term == NULL)
    {
        kdebug_outf("resolve null");
        return newarg;
    }

    switch (term->op_code[0])
    {
        case AML_OP_BYTECONST:
            newarg = kmem_kalloc(sizeof(aml_byteconst));
            memcpy(newarg, term, sizeof(aml_byteconst));
            break;
        case AML_OP_WORDCONST:
            newarg = kmem_kalloc(sizeof(aml_wordconst));
            memcpy(newarg, term, sizeof(aml_wordconst));
            break;
        case AML_OP_DWORDCONST:
            newarg = kmem_kalloc(sizeof(aml_dwordconst));
            memcpy(newarg, term, sizeof(aml_dwordconst));
            break;
        case AML_OP_STRING:
            newarg = kmem_kalloc(sizeof(aml_stringconst));
            memcpy(newarg, term, sizeof(aml_stringconst));
            break;
        case AML_OP_QWORDCONST:
            newarg = kmem_kalloc(sizeof(aml_qwordconst));
            memcpy(newarg, term, sizeof(aml_qwordconst));
            break;
        case AML_OP_ARG0 ... AML_OP_ARG6:
            newarg = kacpi_aml_resolvevalue(aml_arg[term->op_code[0] - 0x68]);
            break;
        case AML_OP_ZERO:
        case AML_OP_ONE:
        case AML_OP_ONES:
            newarg = kmem_kalloc(2);
            newarg->op_code[0] = term->op_code[0];
            break;
        case 0xFE:
            aml_methodinvocation *mi = (aml_methodinvocation *)term;
            if (mi->termlist)
            {
                kacpi_aml_runmethod((aml_method *)mi->method, mi->termlist);
                return aml_returnval;
            }
            else if (mi->method)
                kdebug_outf("FIELD");
            else
                kdebug_outf("FUTURE OBJ");
            break;
        default:
            kdebug_outf("\nunsupported resolve type %x", term->op_code[0]);
            break;
    }

    return newarg;
}

void kacpi_aml_writetarget(aml_termarg *value, aml_target *target)
{
    kdebug_outf("\nwriting [");

    aml_termarg *term_value = kacpi_aml_resolvevalue(value);

    kacpi_aml_printop(term_value);
    kdebug_outf("] into [");
    kacpi_aml_printtarget(target);

    uint8_t value_used = 0;

    switch(target->target_type)
    {
        case AML_TARGET_NULL:
            kdebug_outf("(cant write to ?) (null)");
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
                    uint8_t local_index = target->target_data[0] - 0x60;
                    if (aml_local[local_index] != NULL)
                        kmem_kfree(aml_local[local_index]);
                    aml_local[local_index] = term_value;
                    value_used = 1;
                    break;
                case AML_OP_ARG0 ... AML_OP_ARG6:
                    kdebug_outf("(cant write to ?) ARG%d", target->target_data[0] - 0x68);
                    break;
                default:
                    kdebug_outf("%s", ((char **)target->target_data)[0]);
                    break;
            }
            break;
    }

    kdebug_outf("]");

    if (term_value && value_used == 0)
        kmem_kfree(term_value);
}

void kacpi_aml_runterm(aml_op *op)
{
    if (op == NULL)
        return;

    switch (op->op_code[0])
    {
        case AML_OP_STORE:
            aml_store *store = (aml_store *)op;
            kacpi_aml_writetarget(store->termarg, store->supername);
            break;
        case 0xFE:
            aml_methodinvocation *mi = (aml_methodinvocation *)op;
            if (mi->termlist)
            {
                //kdebug_outf("METHOD");
                kacpi_aml_runmethod((aml_method *)mi->method, mi->termlist);
            }
            else if (mi->method)
                kdebug_outf("FIELD");
            else
                kdebug_outf("FUTURE OBJ");
            break;
        default:
            kdebug_outf("\nrun unknown op %x", op->op_code[0]);
            break;
    }
}

void kacpi_aml_runtermlist(aml_termlist *list)
{
    aml_termlist *list_ptr = list;
    while (list_ptr)
    {
        kacpi_aml_runterm(list_ptr->term_obj);
        list_ptr = list_ptr->next;
    }
}

void kacpi_aml_runmethod(aml_method *method, aml_termlist *list)
{
    kdebug_outf("\nrun method %s", kacpi_aml_gettreename(method->termlist));

    kacpi_aml_printop((aml_op *)method);

    aml_termarg *last_aml_arg[7];
    for (int i = 0; i < 7; i++)
        last_aml_arg[i] = aml_arg[i];

    aml_termlist *list_ptr = list;
    uint8_t arg_num = 0;
    while (list_ptr)
    {
        if (list->term_obj->op_code[0] >= AML_OP_ARG0 &&
            list->term_obj->op_code[0] <= AML_OP_ARG6)
        {
            aml_arg[arg_num] = last_aml_arg[arg_num];
            arg_num++;
        }
        else
            aml_arg[arg_num++] = list->term_obj;
        list_ptr = list_ptr->next;
    }
    while (arg_num < 7)
        aml_arg[arg_num++] = 0;

    kacpi_aml_runtermlist(method->termlist);

    for (int i = 0; i < 7; i++)
        aml_arg[i] = last_aml_arg[i];
}

void kacpi_aml_printdevices()
{
    kdebug_outf("\nkacpi: all acpi devices found [TO-DO]");
}