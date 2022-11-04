#ifndef TABLE_ENTRY_H
#define TABLE_ENTRY_H

#include <stdlib.h>

typedef struct TableEntry
{
    char name[256];
    size_t offset;
    size_t size;
} TableEntry;

#endif