#pragma once

#include <stdint.h>

typedef struct
{
    uint8_t type;
    uint8_t data[];
} kacpi_termarg;

typedef struct
{
    uint8_t encodingvalue[2];
    uint8_t data[];
} kacpi_expression;

typedef struct
{
    uint8_t type;
    uint8_t data[];
} kacpi_datarefobj;

typedef struct
{
    uint64_t* parent;
    uint8_t depth;
    kacpi_expression* obj;
    uint64_t* next;
} kacpi_termlist;

typedef struct
{
    kacpi_termarg *arg;
    uint64_t *next;
} kacpi_termarglist;

typedef uint8_t kacpi_nameseg[4];

typedef struct
{
    uint8_t names;
    kacpi_nameseg name[];
}__attribute__((packed)) kacpi_namepath;

typedef struct
{
    char first;
    kacpi_namepath* namepath;
} kacpi_namestring;

typedef struct
{
    uint8_t type;
    uint8_t data[];
}__attribute__((packed)) kacpi_fieldelement;

typedef struct
{
    kacpi_fieldelement* element;
    uint64_t *next;
} kacpi_fieldlist;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_termarg *buffersize;
    uint8_t* bytelist;
}__attribute__((packed)) kacpi_buffer;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring* namestring;
    kacpi_termlist* termlist;
} kacpi_defscope;

typedef struct
{
    uint8_t op;
    uint64_t *def;
    uint64_t *next;
} kacpi_namespacemodifierobj;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_namestring* namestring;
    uint8_t regionspace;
    kacpi_termarg* regionoffset;
    kacpi_termarg* regionlen;
}__attribute__((packed)) kacpi_defopregion;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring* namestring;
    uint8_t procid;
    uint32_t pblkaddr;
    uint8_t pblklen;
    kacpi_termlist *termlist;
} kacpi_defprocessor;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring* namestring;
    uint8_t fieldflags;
    kacpi_fieldlist* fieldlist;
} kacpi_deffield;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring* namestring1;
    kacpi_namestring* namestring2;
    uint8_t fieldflags;
    kacpi_fieldlist* fieldlist;
} kacpi_defindexfield;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring *namestring;
    uint8_t methodflags;
    kacpi_termlist *termlist;
} kacpi_defmethod;

typedef struct
{
    kacpi_termlist *methodptr;
    uint64_t *next;
} kacpi_methodlist;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_namestring *namestring;
    kacpi_datarefobj *datarefobj;
}__attribute__((packed)) kacpi_defname;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_namestring *namestring1;
    kacpi_namestring *namestring2;
}__attribute__((packed)) kacpi_defalias;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_namestring *namestring;
    kacpi_termlist *termlist;
} kacpi_defdevice;

typedef struct
{
    uint8_t type;
    uint64_t* data;
} kacpi_supername;

typedef struct
{
    uint8_t type;
    uint64_t* data;
} kacpi_target;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand;
    kacpi_target* target;
}__attribute__((packed)) kacpi_deftohexstring;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_termarg* predicate;
    kacpi_termlist* termlist;
} kacpi_defifop;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_termlist* termlist;
} kacpi_defelseop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand1;
    kacpi_termarg* operand2;
    kacpi_target* target;
}__attribute__((packed)) kacpi_deforop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand1;
    kacpi_termarg* operand2;
}__attribute__((packed)) kacpi_deflorop, kacpi_defllessop;

typedef struct
{
    uint8_t type;
    uint64_t* next;
    uint64_t value;
} kacpi_packageelement;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    uint8_t varnumelements;
    uint64_t* packageelementlist;
}__attribute__((packed)) kacpi_defpackageop;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_termarg* varnumelements;
    uint64_t* packageelementlist;
}__attribute__((packed)) kacpi_defvarpackageop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* argobject;
}__attribute__((packed)) kacpi_defreturnop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_namestring *namestring;
    uint8_t syncflags;
}__attribute__((packed)) kacpi_defmutexop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_supername *mutexobject;
    uint16_t timeout;
}__attribute__((packed)) kacpi_defacquireop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_supername *mutexobject;
}__attribute__((packed)) kacpi_defreleaseop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* objreference;
}__attribute__((packed)) kacpi_defderefofop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_supername *supername;
    kacpi_target *target;
}__attribute__((packed)) kacpi_defcondrefof;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg *buffpkgstrobj;
    kacpi_termarg *indexvalue;
    kacpi_target *target;
}__attribute__((packed)) kacpi_defindexop;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand;
    kacpi_target* target;
}__attribute__((packed)) kacpi_deftobuffer;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand;
    kacpi_target* target;
}__attribute__((packed)) kacpi_deffindsetxbit;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand1;
    kacpi_termarg* operand2;
    kacpi_target* target;
}__attribute__((packed)) kacpi_defmathoperation;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* data1;
    kacpi_termarg* data2;
    kacpi_target* target;
}__attribute__((packed)) kacpi_defconcat, kacpi_defconcatres;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand;
    kacpi_supername* supername;
}__attribute__((packed)) kacpi_defstore;

typedef struct
{
    uint8_t encodingvalue[2];
    uint32_t pkglength;
    kacpi_termarg* predicate;
    kacpi_termlist* termlist;
} kacpi_defwhile;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_supername *supername;
}__attribute__((packed)) kacpi_defincrement, kacpi_defdecrement;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* operand;
    kacpi_termarg* count;
    kacpi_target* target;
}__attribute__((packed)) kacpi_defshiftleft, kacpi_defshiftright;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_supername* notifyobject;
    kacpi_termarg* notifyvalue;
}__attribute__((packed)) kapci_defnotify;

typedef struct
{
    uint8_t encodingvalue[2];
    kacpi_termarg* buffer;
    kacpi_termarg* byteindex;
    kacpi_namestring* name;
}__attribute__((packed)) kacpi_defcreatexwordfield;

typedef struct
{
    kacpi_expression* object;
    uint64_t* next;

    uint8_t names;
    char name[];
}__attribute__((packed)) kacpi_tree;

void kacpi_gettermlist(uint32_t length, uint64_t *parent, kacpi_termlist **returnlist);
kacpi_expression* kacpi_getexpression(uint64_t *parent);
uint64_t* kacpi_getpackageelementlist(uint32_t length, uint32_t elements);
kacpi_datarefobj* kacpi_getdatarefobj();
uint8_t kacpi_getbytedata();
kacpi_namepath* kacpi_getnamepath();
kacpi_namestring* kacpi_getnamestring();
uint32_t kacpi_getpkglength();
kacpi_termarg* kacpi_gettermarg(uint64_t *parent);
kacpi_target* kacpi_gettarget(uint64_t *parent);

void kacpi_printtarget(kacpi_target *target);
void kacpi_printtermarg(kacpi_termarg *termarg);
void kacpi_printfield(kacpi_fieldlist *fieldlist, uint8_t fieldflags);
void kacpi_printtermlist(kacpi_termlist *list);
void kacpi_printtermlistentry(kacpi_expression *obj);
void kacpi_printdatarefobj(kacpi_datarefobj *obj);
void kacpi_printnamestring(kacpi_namestring *namestring);