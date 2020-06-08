/*============================================================================
  
  klib
  
  kpath.h

  Definition of the KString class

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

#include <sys/stat.h>
#include <klib/defs.h>
#include <klib/types.h>
#include <klib/kbuffer.h>
#include <klib/kstring.h>

#ifdef WIN32
#define KPATH_SEP_CHAR '\\'
#define KPATH_SEP_UTF8 "\\"
#else
#define KPATH_SEP_CHAR '/'
#define KPATH_SEP_UTF8 "/"
#endif

struct KPath;
typedef struct _KPath KPath;

BEGIN_DECLS

extern KPath   *kpath_clone (const KPath *s);
extern KPath   *kpath_new_from_utf8 (const UTF8 *path);
// Get the value of $HOME. If $HOME is not set, assume root 
extern KPath   *kpath_new_home (void);
extern void     kpath_destroy (KPath *self);

/** Tests with the path ends specifically with a forward-slash, not
    a platform-specific separator. Sometimes separators are forward
    slashes even on platforms where this is not usually the case. */
extern BOOL     kpath_ends_with_fwd_slash (const KPath *self);
extern BOOL     kpath_ends_with_separator (const KPath *self);

extern void     kpath_append (KPath *self, const KPath *s);
extern void     kpath_append_utf8 (KPath *self, const UTF8 *s);
extern void     kpath_append_utf32 (KPath *self, const UTF32 *s);

/** Create the specified directory, and any parent directories that
    are necessary. */
extern BOOL     kpath_create_directory (const KPath *self);

extern FILE    *kpath_fopen (const KPath *self, const char *mode);

extern int      kpath_open_read (const KPath *self);
// Open for write, create, truncate, with default mode
extern int      kpath_open_write (const KPath *self);

extern KBuffer *kpath_read_to_buffer (const KPath *self);
extern KString *kpath_read_to_string (const KPath *self);

/** Remove the filename path of a path, if there is one. This method 
  specifically does not require the path to exist -- it works entirely
  on the name pattern. This makes it possible to use in cases where
  we need to create a file and a directory for it to go it, but it
  means that there are certain ambiguous cases.
  Any path that ends in a separator is assumed to be a directory, and
  is not altered. */
extern void     kpath_remove_filename (KPath *self);

extern BOOL     kpath_size (const KPath *self, int64_t *size);
extern BOOL     kpath_stat (const KPath *self, struct stat *sb);
extern UTF8    *kpath_to_utf8 (const KPath *self);

END_DECLS

