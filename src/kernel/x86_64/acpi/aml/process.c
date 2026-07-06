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

aml_op *kacpi_aml_findtreename(aml_termlist *start, char *name)
{
    aml_termlist *tree_ptr = start;

    char *startname = kacpi_aml_gettreename(tree_ptr);
    kdebug_outf("\nlooking for [%s][%s]", startname, name);

    while (tree_ptr != NULL)
    {
        aml_op *obj = tree_ptr->term_obj;
        if (obj == NULL)
            break;

        kdebug_outf("\nobj %x%x", obj->op_code[0], obj->op_code[1]);

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
                        kdebug_outf("\n[%s][%s] vs [%s][%s]", current_name, ((char **)fieldlist_ptr->element->fielddata)[0], startname, name);
                        if ((str_cmp(current_name, startname) == 0) &&
                            (str_cmp(((char **)fieldlist_ptr->element->fielddata)[0], name) == 0))
                        {
                            kmem_kfree(current_name);
                            kmem_kfree(startname);
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
                kdebug_outf("\n[%s][%s] vs [%s][%s]", current_name, checkname, startname, name);
                if ((str_cmp(current_name, startname) == 0) &&
                    (str_cmp(checkname, name) == 0))
                {
                    kmem_kfree(current_name);
                    kmem_kfree(startname);
                    return obj;
                }
            }
        }
        
        if (current_name)
            kmem_kfree(current_name);

        if (obj->op_code[0] == AML_OP_SCOPE ||
            obj->op_code[0] == AML_OP_METHOD ||
            (obj->op_code[0] == AML_OPEXT_PREFIX && obj->op_code[1] == AML_OPEXT_DEVICE))
        {
            aml_termlist *list = get_termlist(obj);
            kdebug_outf("\nlist %x treeptr %x", list, tree_ptr);
            if (list && (list != tree_ptr))
            {
                aml_op *check = kacpi_aml_findtreename(list, name);
                if (check)
                {   
                    kmem_kfree(startname); 
                    return check;
                }
            }
        }
        
        tree_ptr = tree_ptr->next;
    }

    kmem_kfree(startname);
    return NULL;
}

char *kacpi_aml_gettreename(aml_termlist *tree)
{
    aml_termlist *tree_ptr = tree;
    char *tree_name = NULL;

    kdebug_outf("\nget name ");

    while (tree_ptr)
    {
        if (tree_ptr->listname != 0)
        {
            if (tree_name == NULL)
            {
                tree_name = kmem_kalloc(str_len(tree_ptr->listname) + 1);
                memcpy(tree_name, tree_ptr->listname, str_len(tree_ptr->listname));
            }
            else if (tree_name[0] == AML_PREFIXCHAR)
            {
                size_t prefix = 0;
                while (tree_name[prefix] == AML_PREFIXCHAR)
                    prefix++;

                char *new = kmem_kalloc(str_len(tree_name) - 4 * prefix + str_len(tree_ptr->listname) - prefix + 1);
                memcpy(new, tree_name, str_len(tree_name) - 4 * prefix);
                memcpy(new + str_len(tree_name) - 4 * prefix, tree_ptr->listname + prefix, str_len(tree_ptr->listname) - prefix);
                kmem_kfree(tree_name);
                tree_name = new;
            }
            else
            {
                char *new = kmem_kalloc(str_len(tree_name) + str_len(tree_ptr->listname) + 1);
                memcpy(new + str_len(tree_ptr->listname), tree_name, str_len(tree_name));
                memcpy(new, tree_ptr->listname, str_len(tree_ptr->listname));
                kmem_kfree(tree_name);
                tree_name = new;
            }

            if (tree_name[0] == AML_ROOTCHAR)
                break;
        }

        tree_ptr = tree_ptr->parent;
    }

    if (tree_name != NULL)
        kdebug_outf("[%s]", tree_name);

    return tree_name;
}

void kacpi_aml_printdevices()
{
    kdebug_outf("\nall acpi devices:");
}