#pragma once

#include <stddef.h>
#include <stdint.h>

#define AML_OP_ZERO       0x00
#define AML_OP_ONE        0x01
#define AML_OP_ALIAS      0x06
#define AML_OP_NAME       0x08
#define AML_OP_SCOPE      0x10
#define AML_OP_BUFFER     0x11
#define AML_OP_PACKAGE    0x12
#define AML_OP_VARPACKAGE 0x13
#define AML_OP_METHOD     0x14
#define AML_OP_EXTERNAL   0x15

#define AML_OP_STORE      0x70
#define AML_OP_REFOF      0x71
#define AML_OP_ADD        0x72
#define AML_OP_CONCAT     0x73
#define AML_OP_SUBTRACT   0x74
#define AML_OP_INCREMENT  0x75
#define AML_OP_DECREMENT  0x76
#define AML_OP_MULTIPLY   0x77
#define AML_OP_DIVIDE     0x78
#define AML_OP_SHIFTLEFT  0x79
#define AML_OP_SHIFTRIGHT 0x7A
#define AML_OP_AND        0x7B
#define AML_OP_NAND       0x7C
#define AML_OP_OR         0x7D
#define AML_OP_NOR        0x7E
#define AML_OP_XOR        0x7F
#define AML_OP_NOT        0x80
#define AML_OP_FINDSLBIT  0x81
#define AML_OP_FINDSRBIT  0x82
#define AML_OP_DEREFOF    0x83
#define AML_OP_CONCATRES  0x84
#define AML_OP_MOD        0x85
#define AML_OP_NOTIFY     0x86
#define AML_OP_SIZEOF     0x87
#define AML_OP_INDEX      0x88
#define AML_OP_MATCH      0x89
#define AML_OP_CREATEDWF  0x8A
#define AML_OP_CREATEWF   0x8B
#define AML_OP_CREATEBYF  0x8C
#define AML_OP_CREATEBIF  0x8D
#define AML_OP_OBJTYPE    0x8E
#define AML_OP_CREATEQWF  0x8F
#define AML_OP_LAND       0x90
#define AML_OP_LOR        0x91
#define AML_OP_LNOT       0x92
#define AML_OP_LEQUAL     0x93
#define AML_OP_LGREATER   0x94
#define AML_OP_LLESS      0x95
#define AML_OP_TOBUFFER   0x96
#define AML_OP_TODECSTR   0x97
#define AML_OP_TOHEXSTR   0x98
#define AML_OP_TOINT      0x99
#define AML_OP_TOSTR      0x9C
#define AML_OP_COPYOBJECT 0x9D
#define AML_OP_MID        0x9E
#define AML_OP_CONTINUE   0x9F
#define AML_OP_IF         0xA0
#define AML_OP_ELSE       0xA1
#define AML_OP_WHILE      0xA2
#define AML_OP_NOOP       0xA3
#define AML_OP_RETURN     0xA4
#define AML_OP_BREAK      0xA5
#define AML_OP_BREAKPOINT 0xCC
#define AML_OP_ONES       0xFF

#define AML_OPEXT_PREFIX      0x5B
#define AML_OPEXT_MUTEX       0x01
#define AML_OPEXT_EVENT       0x02
#define AML_OPEXT_CONDREFOF   0x12
#define AML_OPEXT_CREATEFIELD 0x13
#define AML_OPEXT_LOADTABLE   0x1F
#define AML_OPEXT_LOAD        0x20
#define AML_OPEXT_STALL       0x21
#define AML_OPEXT_SLEEP       0x22
#define AML_OPEXT_ACQUIRE     0x23
#define AML_OPEXT_SIGNAL      0x24
#define AML_OPEXT_WAIT        0x25
#define AML_OPEXT_RESET       0x26
#define AML_OPEXT_RELEASE     0x27
#define AML_OPEXT_FROMBCD     0x28
#define AML_OPEXT_TOBCD       0x29
#define AML_OPEXT_UNLOAD      0x2A
#define AML_OPEXT_REVISION    0x30
#define AML_OPEXT_DEBUG       0x31
#define AML_OPEXT_FATAL       0x32
#define AML_OPEXT_TIMER       0x33
#define AML_OPEXT_OPREGION    0x80
#define AML_OPEXT_FIELD       0x81
#define AML_OPEXT_DEVICE      0x82
#define AML_OPEXT_PROCESSOR   0x83
#define AML_OPEXT_POWERRES    0x84
#define AML_OPEXT_THERM_ZL    0x85
#define AML_OPEXT_INDEXFIELD  0x86
#define AML_OPEXT_BANKFIELD   0x87
#define AML_OPEXT_DATAREGION  0x88

#define AML_ROOTCHAR   0x5C
#define AML_PREFIXCHAR 0x5E
#define AML_NAMECHAR   0x5F

#define AML_OP_LOCAL0 0x60
#define AML_OP_LOCAL1 0x61
#define AML_OP_LOCAL2 0x62
#define AML_OP_LOCAL3 0x63
#define AML_OP_LOCAL4 0x64
#define AML_OP_LOCAL5 0x65
#define AML_OP_LOCAL6 0x66
#define AML_OP_LOCAL7 0x67
#define AML_OP_ARG0   0x68
#define AML_OP_ARG1   0x69
#define AML_OP_ARG2   0x6A
#define AML_OP_ARG3   0x6B
#define AML_OP_ARG4   0x6C
#define AML_OP_ARG5   0x6D
#define AML_OP_ARG6   0x6E

#define AML_OP_BYTECONST  0x0A
#define AML_OP_WORDCONST  0x0B
#define AML_OP_DWORDCONST 0x0C
#define AML_OP_STRING     0x0D
#define AML_OP_QWORDCONST 0x0E

#define AML_NAME_NULL  0x00
#define AML_NAME_DUAL  0x2E
#define AML_NAME_MULTI 0x2F

#define AML_TARGET_NULL      0x00
#define AML_TARGET_NAME      0x01
#define AML_TARGET_DEBUG     0x02
#define AML_TARGET_REFERENCE 0x03

#define AML_FIELD_RESERVED 0x00
#define AML_FIELD_ACCESS   0x01
#define AML_FIELD_CONNECT  0x02
#define AML_FIELD_EXACCESS 0x03

typedef struct
{
    uint8_t op_code[2];
    uint8_t op_data[];
} aml_op, aml_termarg;

typedef struct
{
    uint8_t target_type;
    uint8_t target_data[];
} aml_target;

typedef struct aml_termlist_s
{
    struct aml_termlist_s *parent;
    struct aml_termlist_s *front;
    char *fullname;
    char *listname;
    aml_op *term_obj;
    struct aml_termlist_s *next;
} aml_termlist;

typedef struct
{
    uint8_t fieldtype;
    uint32_t len;
} aml_fieldelement_resv;

typedef struct
{
    uint8_t fieldtype;
    uint8_t data[2];
} aml_fieldelement_access;

typedef struct
{
    uint8_t fieldtype;
    char *name;
    uint32_t len;
} aml_fieldelement_default;

typedef struct
{
    uint8_t fieldtype;
    aml_op *value;
} aml_packageelement_op;

typedef struct
{
    uint8_t fieldtype;
    char *name;
} aml_packageelement_name;

typedef struct
{
    uint8_t fieldtype;
    uint8_t fielddata[];
} aml_fieldelement, aml_packageelement;

typedef struct aml_fieldlist_s
{
    aml_fieldelement *element;
    struct aml_fieldlist_s *next;
} aml_fieldlist, aml_packagelist;

typedef struct
{
    uint8_t op_code[2];
    aml_op *method;
    char *namestring;
    aml_termlist *termlist;
} aml_methodinvocation;

typedef struct
{
    uint8_t op_code[2];
    char *namestring1;
    char *namestring2;
} aml_alias;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    aml_termarg *datarefobj;
} aml_name;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    aml_termlist *termlist;
} aml_scope;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    aml_termarg *buffersize;
    uint8_t *bytelist;
} aml_buffer;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    uint8_t numelements;
    aml_packagelist *packageelementlist;
} aml_package;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    aml_termarg *varnumelements;
    aml_packagelist *packageelementlist;
} aml_varpackage;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    uint8_t methodflags;
    aml_termlist *termlist;
    uint64_t start;
    uint32_t termlength;
} aml_method;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    uint8_t objecttype;
    uint8_t argumentcount;
} aml_external;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *termarg;
    aml_target *supername;
} aml_store;

typedef struct
{
    uint8_t op_code[2];
    aml_target *supername;
} aml_refof, aml_increment, aml_decrement, aml_sizeof, aml_objecttype;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand1;
    aml_termarg *operand2;
    aml_target *target;
} aml_add, aml_concat, aml_subtract, aml_multiply, aml_shiftleft,
aml_shiftright, aml_and, aml_nand, aml_or, aml_nor, aml_xor,
aml_concatres, aml_mod, aml_index, aml_tostring;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *dividend;
    aml_termarg *divisor;
    aml_target *remainder;
    aml_target *quotient;
} aml_divide;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand;
    aml_target *target;
} aml_not, aml_findsetleftbit, aml_findsetrightbit, aml_tobuffer,
aml_todecimalstring, aml_tohexstring, aml_tointeger;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *objreference;
} aml_derefof, aml_return;

typedef struct
{
    uint8_t op_code[2];
    aml_target *notifyobject;
    aml_termarg *notifyvalue;
} aml_notify;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *searchpkg;
    uint8_t matchopcode1;
    aml_termarg *operand1;
    uint8_t matchopcode2;
    aml_termarg *operand2;
    aml_termarg *startindex;
} aml_match;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *sourcebuff;
    aml_termarg *index;
    char *namestring;
} aml_createdwordfield, aml_createwordfield, aml_createbytefield,
aml_createbitfield, aml_createqwordfield;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand1;
    aml_termarg *operand2;
} aml_land, aml_lor, aml_lequal,
aml_lgreater, aml_lless;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand;
} aml_lnot;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand;
    aml_target *simplename;
} aml_copyobject;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand1;
    aml_termarg *operand2;
    aml_termarg *operand3;
    aml_target *target;
} aml_mid;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    aml_termarg *predicate;
    aml_termlist *termlist;
    aml_op *defelse;
} aml_if;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    aml_termlist *termlist;
} aml_else;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    aml_termarg *predicate;
    aml_termlist *termlist;
} aml_while;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    uint8_t syncflags;
} aml_mutex;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
} aml_event;

typedef struct
{
    uint8_t op_code[2];
    aml_target *supername;
    aml_target *target;
} aml_condrefof;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *sourcebuff;
    aml_termarg *bitindex;
    aml_termarg *numbits;
    char *namestring;
} aml_createfield;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *term1;
    aml_termarg *term2;
    aml_termarg *term3;
    aml_termarg *term4;
    aml_termarg *term5;
    aml_termarg *term6;
} aml_loadtable;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    aml_target *target;
} aml_load;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *time;
} aml_stall, aml_sleep;

typedef struct
{
    uint8_t op_code[2];
    aml_target *mutexobject;
    uint16_t timeout;
} aml_acquire;

typedef struct
{
    uint8_t op_code[2];
    aml_target *eventobject;
} aml_signal, aml_reset, aml_release;

typedef struct
{
    uint8_t op_code[2];
    aml_target *eventobject;
    aml_termarg *operand;
} aml_wait;

typedef struct
{
    uint8_t op_code[2];
    aml_termarg *operand;
    aml_target *target;
} aml_frombcd, aml_tobcd;

typedef struct
{
    uint8_t op_code[2];
    uint8_t fataltype;
    uint32_t fatalcode;
    aml_termarg *fatalarg;
} aml_fatal;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    uint8_t regionspace;
    aml_termarg *regionoffset;
    aml_termarg *regionlen;
} aml_opregion;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    uint8_t fieldflags;
    aml_fieldlist *fieldlist;
} aml_field;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    aml_termlist *termlist;
} aml_device, aml_thermalzone;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    uint8_t id;
    uint32_t pblkaddr;
    uint8_t pblklen;
    aml_termlist *termlist;
} aml_processor;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring;
    uint8_t systemlevel;
    uint16_t resourceorder;
    aml_termlist *termlist;
} aml_powerres;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring1;
    char *namestring2;
    uint8_t fieldflags;
    aml_fieldlist *fieldlist;
} aml_indexfield;

typedef struct
{
    uint8_t op_code[2];
    uint32_t pkglength;
    char *namestring1;
    char *namestring2;
    aml_termarg *bankvalue;
    uint8_t fieldflags;
    aml_fieldlist *fieldlist;
} aml_bankfield;

typedef struct
{
    uint8_t op_code[2];
    char *namestring;
    aml_termarg *termarg1;
    aml_termarg *termarg2;
    aml_termarg *termarg3;
} aml_dataregion;

typedef struct
{
    uint8_t op_code[2];
    uint8_t byteconst;
} aml_byteconst;

typedef struct
{
    uint8_t op_code[2];
    uint16_t wordconst;
} aml_wordconst;

typedef struct
{
    uint8_t op_code[2];
    uint32_t dwordconst;
} aml_dwordconst;

typedef struct
{
    uint8_t op_code[2];
    char *stringconst;
} aml_stringconst;

typedef struct
{
    uint8_t op_code[2];
    uint64_t qwordconst;
} aml_qwordconst;

aml_op *kacpi_aml_findtreename(aml_termlist *start, char *fullname);
char *kacpi_aml_gettreename(aml_termlist *tree);
void kacpi_aml_runmethod(aml_method *method, aml_termlist *list);
void kacpi_aml_printdevices();

void kacpi_aml_generatetree(uint8_t *aml_ptr, size_t length, aml_termlist *tree);
void kacpi_aml_printtermlist(aml_termlist *tl);
void kacpi_aml_printtarget(aml_target *target);
void kacpi_aml_printop(const aml_op *op);

void kacpi_processdsdt(uint64_t dsdt_addr);
int kacpi_aml_intfromop(aml_op *op, uint64_t *value);
aml_termlist *get_termlist(aml_op *op);

extern aml_termlist *kacpi_aml_root;