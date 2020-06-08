/*============================================================================
  
  klib
  
  klist.h

  Definition of the KList class

  This class holds a linked list of references. Once added, the references
  "belong" to the list, and should not be called or modified except 
  by removing them from the list, or destroying the list. 

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

#include <klib/defs.h>
#include <klib/types.h>

struct KList;
typedef struct _KList KList;

BEGIN_DECLS

// The comparison function should return -1, 0, +1, like strcmp. In practice
//   however, the functions that use this only care whether two things 
//   are equal -- ordering is not important. The i1,i2 arguments are 
//   pointers to the actual objects in the list. user_data is not used
//   at present
typedef int (*ListCompareFn) (const void *i1, const void *i2,
          void *user_data);

typedef void (*KListFreeFn) (void *);

extern KList *klist_new_empty (KListFreeFn free_fn);
extern void   klist_destroy (KList *self);

extern void   klist_append (KList *self, void *ref);
extern void   klist_clear (KList *self);
extern void  *klist_get (const KList *self, size_t i);
extern size_t klist_length (const KList *self);


/** Remove all items from the last that are a match for 'item', as
determined by a comparison function.

It is necessary to provide a comparison function, and items will be
removed (and freed) that satisfy the comparison function. 

IMPORTANT -- The "item" argument cannot be a direct reference to an
item already in the list. If that item is removed from the list its
memory will be freed. The "item" argument will thus be an invalid
memory reference, and the program will crash when it is next used. 

To remove one specific, known, item from the list, use klist_remove_ref()
*/
extern void   klist_remove (KList *self, const void *item, 
                ListCompareFn fn);

/*Remove the specific item from the list, if it is present. The object's
remove function will be called. This method can't be used to remove an
object by value -- that is, you can't pass "dog" to the method to remove
all strings whose value is "dog". Use klist_remove() for that.*/
extern void   klist_remove_ref (KList *self, const void *ref);

END_DECLS
