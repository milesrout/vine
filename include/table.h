#ifndef VINE_ALLOC_H_INCLUDED
#error "Must include alloc.h before including table.h"
#endif
#ifndef VINE_HASH_H_INCLUDED
#error "Must include hash.h before including table.h"
#endif
#ifdef VINE_TABLE_H_INCLUDED
#error "May not include table.h more than once"
#endif
#define VINE_TABLE_H_INCLUDED
/* Hash tables are an important part of the implementation of any scripting
 * language.
 */
