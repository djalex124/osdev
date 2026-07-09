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
    //kdebug_outf("\nlooking for [%s][%s]", startname, name);

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
                        //kdebug_outf("\n[%s][%s] vs [%s][%s]", current_name, fieldname, startname, name);
                        if (((str_cmp(current_name, startname) == 0) &&
                            (str_cmp(fieldname, name) == 0)))
                        {
                            kmem_kfree(current_name);
                            kmem_kfree(startname);
                            return obj;
                        }
                        else if (name[0] == AML_ROOTCHAR && (str_len(current_name) + 4) == str_len(name))
                        {
                            if ((strn_cmp(current_name, name, str_len(current_name)) == 0) &&
                                strn_cmp(fieldname, name + str_len(current_name), 4) == 0)
                            {
                                kmem_kfree(current_name);
                                kmem_kfree(startname);
                                return obj;
                            }
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
                //kdebug_outf("\n[%s][%s] vs [%s][%s]", current_name, checkname, startname, name);
                if ((str_cmp(current_name, startname) == 0) &&
                    (str_cmp(checkname, name) == 0))
                {
                    kmem_kfree(current_name);
                    kmem_kfree(startname);
                    return obj;
                }
                else if (name[0] == AML_ROOTCHAR && (str_len(current_name) + 4) == str_len(name))
                {
                    if ((strn_cmp(current_name, name, str_len(current_name)) == 0) &&
                        strn_cmp(checkname, name + str_len(current_name), 4) == 0)
                    {
                        kmem_kfree(current_name);
                        kmem_kfree(startname);
                        return obj;
                    }
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
            //kdebug_outf("\nlist %x treeptr %x", list, tree_ptr);
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

    //kdebug_outf("\nget name ");

    while (tree_ptr)
    {
        if (tree_ptr->listname != 0)
        {
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
                //kdebug_outf("\nsolving prefix?");

                size_t prefix = 0;
                while (tree_name[prefix] == AML_PREFIXCHAR)
                    prefix++;

                char *name = tree_name + prefix;

                while (prefix > 0 && tree_ptr)
                {
                    if (tree_ptr->listname)
                        prefix--;
                    tree_ptr = tree_ptr->parent;
                }

                char *prefix_name = kacpi_aml_gettreename(tree_ptr);

                char *new = kmem_kalloc(str_len(prefix_name) + str_len(name) + 1);
                memcpy(new, prefix_name, str_len(prefix_name));
                memcpy(new + str_len(prefix_name), name, str_len(name));

                kmem_kfree(tree_name);
                kmem_kfree(prefix_name);

                return new;
            }
        }

        if (tree_name[0] == AML_ROOTCHAR)
            break;
        tree_ptr = tree_ptr->parent;
    }

    //if (tree_name != NULL)
    //    kdebug_outf("[%s]", tree_name);

    return tree_name;
}

void kacpi_aml_runtermlist(aml_op *op)
{
    if (op == NULL)
        return;
    
    //kdebug_outf("\n op %x %x", op->op_code[0], op->op_code[1]);

    switch (op->op_code[0])
    {
        case 0xFE:
            aml_methodinvocation *mi = (aml_methodinvocation *)op;
            if (mi->termlist)
            {
                kdebug_outf("METHOD");
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

void kacpi_aml_runmethod(aml_method *method, aml_termlist *list)
{
    char *name = kacpi_aml_gettreename(method->termlist);
    kdebug_outf("\nrun method %s", name);
    kmem_kfree(name);

    aml_termlist *list_ptr = method->termlist;
    while (list_ptr)
    {
        kacpi_aml_runtermlist(list_ptr->term_obj);
        list_ptr = list_ptr->next;
    }
}

void kacpi_aml_printdevices()
{
    kdebug_outf("\nall acpi devices:");
}