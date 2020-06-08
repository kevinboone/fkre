/*============================================================================
  
  klib
  
  kpath.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "convertutf.h" 
#include <klib/kstring.h>
#include <klib/kpath.h>
#include <klib/klog.h>
#include <klib/kbuffer.h>

#define KLOG_CLASS "klib.kpath"

/*============================================================================
  
  kpath_clone

  ==========================================================================*/
KPath *kpath_clone (const KPath* path) 
  {
  KLOG_IN
  KPath *ret = (KPath *)kstring_strdup ((KString *)path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_new_utf8 

  ==========================================================================*/
KPath *kpath_new_from_utf8 (const UTF8* path) 
  {
  KLOG_IN
  KPath *ret = (KPath *)kstring_new_from_utf8 (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_new_home

  ==========================================================================*/
KPath *kpath_new_home (void)
  {
  KLOG_IN
  const char *home = getenv ("HOME");
  klog_debug (KLOG_CLASS, "Initializing KPath from home directory '%s'", home);
  if (home == NULL) home = KPATH_SEP_UTF8; 
  KLOG_OUT
  return kpath_new_from_utf8 ((UTF8*)home);
  }

/*============================================================================
  
  kpath_destroy

  ==========================================================================*/
void kpath_destroy (KPath *self)
  {
  KLOG_IN
  kstring_destroy ((KString *)self);
  KLOG_OUT
  }

/*============================================================================
  
  kpath_append

  ==========================================================================*/
void kpath_append (KPath *self, const KPath *s)
  {
  KLOG_IN
  UTF8 *temp = kstring_to_utf8 ((KString *)s);
  kpath_append_utf8 (self, temp);
  free (temp);
  KLOG_OUT
  }

/*============================================================================
  
  kpath_append_utf8

  ==========================================================================*/
void kpath_append_utf8 (KPath *self, const UTF8 *s)
  {
  KLOG_IN
  // Empty string is a special case -- append without a separator
  // This is conventionally a relative file
  if (kstring_length ((KString *)self) == 0)
    {
    kstring_append_printf ((KString *)self, "%s", s);
    }
  else
    {
    if (!kstring_ends_with_utf8 ((KString *)self, (UTF8*)KPATH_SEP_UTF8))
      {
      kstring_append_utf8 ((KString *)self, (UTF8*) KPATH_SEP_UTF8);
      }
    if (s[0] == KPATH_SEP_CHAR)
      kstring_append_printf ((KString *)self, "%s", s + 1);
    else
      kstring_append_printf ((KString *)self, "%s", s);
    }
  KLOG_OUT
  }

/*============================================================================
  
  kpath_append_utf32

  ==========================================================================*/
void kpath_append_utf32 (KPath *self, const UTF32 *s)
  {
  KLOG_IN
  KString *temp = kstring_new_from_utf32 (s);
  kpath_append (self, (KPath *)temp);
  kstring_destroy (temp);
  KLOG_OUT
  }

/*============================================================================
  
  kpath_ends_with_fwd_slash

  ==========================================================================*/
BOOL kpath_ends_with_fwd_slash (const KPath *self)
  {
  KLOG_IN
  static UTF32 slash[] = {'/', 0};
  BOOL ret = kstring_ends_with_utf32 ((KString *)self, slash);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_ends_with_separator

  ==========================================================================*/
extern BOOL kpath_ends_with_separator (const KPath *self)
  {
  KLOG_IN
  BOOL ret = (kstring_ends_with_utf8 ((KString *)self, 
                   (UTF8*)KPATH_SEP_UTF8));
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_create_directory

  This is a bit nasty -- I haven't had time to code all the logic
  myself, so this method just invokes "mkdir -p". This is a reasonable
  this to do on Unix-like systems, but perhaps not elsewhere. A problem
  with working this way is it is a little awkward to tell when the
  directory creation has failed.

  ==========================================================================*/
BOOL kpath_create_directory (const KPath *self)
  {
  KLOG_IN
  BOOL ret = FALSE; 
  UTF8 *path = kstring_to_utf8 ((KString *)self);

  pid_t pid = fork();

  if (pid != -1)
    {
    if (pid > 0)
      {
      int status;
      waitpid (pid, &status, 0);
      if (status == 0) ret = TRUE;
      errno = status;
      }
    else
      {
      // we are the child
      execlp ("mkdir", "mkdir", "-p", path, NULL);
      _exit (EXIT_FAILURE);   // exec never returns
      }
    }
  // else fork() failed

  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_fopen

  ==========================================================================*/
FILE *kpath_fopen (const KPath *self, const char *mode)
  {
  KLOG_IN
  FILE *ret = NULL;
  UTF8 *path = kstring_to_utf8 ((KString *)self);
  ret = fopen ((char *)path, mode);
  free (path);
  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  kpath_open_read

  ==========================================================================*/
int kpath_open_read (const KPath *self)
  {
  KLOG_IN
  int ret = 0;  
  UTF8 *path = kstring_to_utf8 ((KString *)self);
  klog_debug (KLOG_CLASS, "Open '%s' for read", path);
  ret = open ((char *)path, O_RDONLY);
  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_open_write

  ==========================================================================*/
int kpath_open_write (const KPath *self)
  {
  KLOG_IN
  int ret = 0;  
  UTF8 *path = kstring_to_utf8 ((KString *)self);
  klog_debug (KLOG_CLASS, "Open '%s' for write", path);
  ret = open ((char *)path, O_WRONLY | O_CREAT | O_TRUNC);
  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_read_to_buffer

  ==========================================================================*/
KBuffer *kpath_read_to_buffer (const KPath *self)
  {
  KLOG_IN
  KBuffer *ret = NULL;
  char *path = (char *)kstring_to_utf8 ((KString *)self);
  klog_debug (KLOG_CLASS, "Read to buffer from '%s'", path);

  int64_t size;
  if (kpath_size (self, &size))
    {
    BYTE *b = malloc (size + sizeof (uint32_t));
    if (b)
      {
      int fd = kpath_open_read (self);
      if (fd >= 0)
        {
        int n = read (fd, b, size);
        if (n == size)
          {
          ret = kbuffer_new_from_data (b, size);
          free (b);  
          }
        else
          {
          klog_error (KLOG_CLASS, "Incomplete read -- wanted %ld, got %ld",   
             size, n);
          errno = EIO;
          }
        }
      }
    else
      {
      klog_debug (KLOG_CLASS, "malloc(%ld) failed: %s",   
         size, strerror (errno));
      }
    }
  else
    {
    klog_debug (KLOG_CLASS, "Can't get size of '%s': %s", path,   
       strerror (errno));
    }

  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_read_to_string

  ==========================================================================*/
KString *kpath_read_to_string (const KPath *self)
  {
  KLOG_IN
  KString *ret = NULL;
  char *path = (char *)kstring_to_utf8 ((KString *)self);
  klog_debug (KLOG_CLASS, "Read to string from '%s'", path);

  KBuffer *buff = kpath_read_to_buffer (self);
  if (buff)
    {
    ret = kstring_new_from_utf8 (kbuffer_get_data (buff));
    kbuffer_destroy (buff);
    }

  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_remove_filename

  ==========================================================================*/
void kpath_remove_filename (KPath *self)
  {
  KLOG_IN
  assert (self != NULL);
  if (kpath_ends_with_separator (self))
    {
    // Assume there is already no filename
    }
  else
    {
    printf ("sep = %S\n", kstring_cstr ((KString *)self));
    int p = kstring_find_last_utf8 ((const KString *)self, 
               (UTF8*)KPATH_SEP_UTF8);
    if (p >= 0)
      {
      int delete_from = p + 1;
      int to_delete = kstring_length ((KString *)self) - delete_from;
      kstring_delete ((KString *)self, delete_from, to_delete);
      }
    else
      {
      // No separator. This should never really happen, but if the path is
      //   simply 'foo', there's no way to know (if it isn't actually 
      //   a file that already exists) whether it's a filename or a 
      //   directory. So do nothing :/
      }
    }
 

  KLOG_OUT
  }

/*============================================================================
  
  kpath_size

  ==========================================================================*/
BOOL kpath_size (const KPath *self, int64_t *size)
  {
  KLOG_IN
  char *path = (char *)kstring_to_utf8 ((KString *)self);
  BOOL ret = FALSE;
  klog_debug (KLOG_CLASS, "Getting size of '%s'", path);

  struct stat sb;
  if (kpath_stat (self, &sb))
    {
    *size = sb.st_size;
    ret = TRUE;
    }
  else
    {
    // Message should have been displayed by _stat()
    ret = FALSE;
    }

  free (path);
  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  kpath_stat

  ==========================================================================*/
BOOL kpath_stat (const KPath *self, struct stat *sb)
  {
  KLOG_IN
  char *path = (char *)kstring_to_utf8 ((KString *)self);
  klog_debug (KLOG_CLASS, "Calling stat() on '%s'", path);

  BOOL ret;
  if (stat (path, sb) == 0)
    ret = TRUE;
  else
    {
    klog_debug (KLOG_CLASS, "stat() failed for '%s': %s", path,   
       strerror (errno));
    ret = FALSE;
    }

  free (path);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  kpath_to_utf8

  ==========================================================================*/
UTF8 *kpath_to_utf8 (const KPath *self)
  {
  KLOG_IN
  UTF8 *ret = kstring_to_utf8 ((KString *)self); 
  KLOG_OUT
  return ret;
  }




