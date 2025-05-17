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
    uint64_t* next;
} kacpi_termobj;

typedef struct
{
    kacpi_termobj* obj;
} kacpi_termlist;

typedef char kacpi_nameseg[4];

typedef struct
{
    uint8_t names;
    kacpi_nameseg name[];
} kacpi_namepath;

typedef struct
{
    char first;
    kacpi_namepath* namepath;
} kacpi_namestring;

typedef struct
{
    uint8_t type;
    uint64_t *next;
    uint8_t data[];
} kacpi_fieldelement;

typedef struct
{
    kacpi_fieldelement* elements;
} kacpi_field;

typedef struct
{
    uint32_t pkglength;
    kacpi_namestring* namestring;
    kacpi_termlist* termlist;
} kacpi_defscope;

typedef struct
{
    uint8_t op;
    uint64_t *def;
} kacpi_namespacemodifierobj;

typedef struct
{
    uint8_t encodingvalue[2];
    uint64_t* next;
    kacpi_namestring* namestring;
    uint8_t regionspace;
    kacpi_termarg* regionoffset;
    kacpi_termarg* regionlen;
} kacpi_defopregion;

typedef struct
{
    uint8_t encodingvalue[2];
    uint64_t* next;
    uint32_t pkglength;
    kacpi_namestring* namestring;
    uint8_t fieldflags;
    kacpi_field* fieldlist;
} kacpi_deffield;

typedef struct
{
    
} kacpi_supername;

typedef struct
{
    uint8_t type;
    uint8_t* string;
} kacpi_target;

typedef struct
{
    uint8_t encodingvalue[2];
    uint64_t* next;
    kacpi_termarg* operand;
    kacpi_target* target;
} kacpi_deftohexstring;

kacpi_termlist* kacpi_gettermlist();
uint8_t kacpi_getbytedata();
kacpi_namepath* kacpi_getnamepath();
kacpi_namestring* kacpi_getnamestring();
uint32_t kacpi_getpkglength();