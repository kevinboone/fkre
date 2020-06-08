/*============================================================================
  
  FKRE 
  
  main.c

  Flesch-Kincaid

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <getopt.h> 
#include <klib/klib.h> 

// These are the code points of characters we will treat as vowel sounds,
//   for the purposes of splitting a word into syllables
#define VOWELS  { 'a', 'e', 'i', 'o', 'u', 'y', \
                     0xE1 /*'á'*/, 0xE9 /*'é'*/, 0xEF /*'ï'*/}

#define KLOG_CLASS "fkre"

// Types and state for the finite-state machine used to split text 

typedef enum 
  {
  STATE_START = 0, 
  STATE_TAG = 1,
  STATE_WHITE = 2,
  STATE_TEXT = 3 
  } State;

typedef enum 
  {
  TYPE_UNKNOWN = 0, 
  TYPE_STARTTAG = 1, 
  TYPE_ENDTAG = 2, 
  TYPE_WHITE = 3, 
  TYPE_TEXT = 4, 
  } Type;

typedef struct _FKREContext
  {
  BOOL html;
  int words;
  int sentences;
  int current_sentence_length;
  int max_sentence_length;
  int syllables;
  double score;
  int words_in_this_subheading;
  int subheadings;
  int maximum_words_per_subheading;
  KString *last_word;
  int passive_sentences;
  } FKREContext;

/*============================================================================
  
  fkre_log_handler

  ==========================================================================*/
void fkre_log_handler (KLogLevel level, const char *cls, 
                  void *user_data, const char *msg)
  {
  fprintf (stderr, "%s %s: %s\n", klog_level_to_utf8 (level), cls, msg);
  // TODO -- format log better
  }


/*============================================================================
  
  fkre_count_syllables

  Split a word into syllables. The word is assumed to consist only of
  pronounceable letters. The algorithm is very simple -- essentially a
  syllable is a group of consonants separated by a group of vowels.

  There are far more accurate ways to count syllables but, since all we
  care about here is the average number of syllables per word, it hardly
  seems worth burning a heap of extra CPU cycles.

  ==========================================================================*/
int fkre_count_syllables (const KString *word)
  {
  static UTF32 vowels[] = VOWELS; 
  static int nvowels = sizeof (vowels) / sizeof (UTF32);
  int n = 0;
  BOOL last_vowel = FALSE;
  int l = kstring_length (word); 
  for (int i = 0 ; i < l; i++)
    {
    UTF32 wc = kstring_get (word, i);
    BOOL got_vowel = FALSE;
    for (int j = 0; j < nvowels; j++)
      {
      UTF32 v = vowels[j];
      // Convert to lower case
      if (v >= 65 && v <= 90) v += 32; // ASCII
      if (v >= 192 && v <= 222) v += 32; // Extended latin
      if ((v == wc) && last_vowel)
        {
        got_vowel = TRUE;
        last_vowel = TRUE;
        break;
        }
      else if (v == wc && !last_vowel)
        {
        n++;
        got_vowel = TRUE;
        last_vowel = TRUE;
        break;
        }
      }
    if (!got_vowel)
      last_vowel = FALSE;
    }
  // 'es' on the end of a work is often not sounded as an extra syllable
  if (kstring_ends_with_utf8 (word, (UTF8 *)"es")) 
    n--;
  // 'e' on the end of a work is usually not sounded
  else if (kstring_ends_with_utf8 (word, (UTF8 *)"e"))
    n--;
  return n;
  }


/*============================================================================
  
  fkre_classify 

  ==========================================================================*/
Type fkre_classify (BOOL html, int c)
  {
  KLOG_IN
  Type ret = TYPE_UNKNOWN;

  if (html)
    {
    if (c == '<') ret = TYPE_STARTTAG; 
    if (c == '>') ret = TYPE_ENDTAG; 
    }

  if (ret == TYPE_UNKNOWN)
    {
    // Various Unicode spaces
    if (c >= 0x2000 && c <= 0x200A)
      {
      ret = TYPE_WHITE;
      }
    }

  if (ret == TYPE_UNKNOWN)
    {
    switch (c)
      {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
      case 0x0B: // Line tab
      case 0x85: // Next line 
      case 0xA0: // NBSP 
      case 0x2028: // Line sep 
      case 0x2029: // Para sep 
      case 0x202F: // Narrow NBSP 
        ret = TYPE_WHITE;
        break;
      }
    }

  if (ret == TYPE_UNKNOWN)
    ret = TYPE_TEXT;
  return ret;
  KLOG_OUT
  }

/*============================================================================
  
  fkre_got_subheading

  ==========================================================================*/
void fkre_got_subheading (FKREContext *context)
  {
  if (context->words_in_this_subheading > context->maximum_words_per_subheading)
    context->maximum_words_per_subheading = context->words_in_this_subheading;
  context->words_in_this_subheading = 0;
  }


/*============================================================================
  
  fkre_do_tag

  ==========================================================================*/
void fkre_do_tag (FKREContext *context, const KString *tag)
  {
  KLOG_IN
  // Be aware that tags have attributes
  // printf ("** TAG %S\n", kstring_cstr (tag));

  if (kstring_length(tag) >= 1)
    {
    if (kstring_get (tag, 0) == 'h'
       || kstring_get (tag, 0) == 'H')
      {
      int c1 = kstring_get (tag, 1);
      if (c1 >= '1' && c1 <= '9')
        {
        context->subheadings++; 
        fkre_got_subheading (context); 
        }
      }
    }

  KLOG_OUT
  }

/*============================================================================
  
  fkre_extract_letters

  ==========================================================================*/
KString *fkre_extract_letters (const KString *word)
  {
  KLOG_IN
  KString *ret = kstring_new_empty();
  int l = kstring_length (word);
  for (int i = 0; i < l; i++)
    {
    UTF32 c = kstring_get (word, i);
    if (
       (c >= 'a' && c <= 'z') ||
       (c >= 'A' && c <= 'Z') ||
       (c >= 192 && c <= 255) // iso-8859-1 extended latin 
       )
    kstring_append_char (ret, c);
    }
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  fkre_do_word

  ==========================================================================*/
void fkre_do_word (FKREContext *context, const KString *word)
  {
  KLOG_IN
  // TODO remove HTML entities
  klog_debug (KLOG_CLASS, "Got word %S", kstring_cstr (word));

  BOOL end_sentence = FALSE;
  if (kstring_ends_with_utf8 (word, (UTF8*)".")) 
    end_sentence = TRUE;
  if (kstring_ends_with_utf8 (word, (UTF8*)"?")) 
    end_sentence = TRUE;
  if (end_sentence)
    {
    klog_debug (KLOG_CLASS, "sentence length: %d",   
                  context->current_sentence_length);
    if (context->current_sentence_length > context->max_sentence_length)
      context->max_sentence_length = context->current_sentence_length;
    context->sentences++;
    context->current_sentence_length = 0;
    }

  // Now we've figured out whether this word ends a sentence or not,
  //  strip all but letters.

  KString *clean_word = fkre_extract_letters (word);
  klog_debug (KLOG_CLASS, "Depunctuated word %S",  
               kstring_cstr (clean_word));
  if (kstring_length (clean_word) > 0)
    {
    int syls = fkre_count_syllables (clean_word);
    context->syllables += syls;
    context->current_sentence_length++;
    context->words++;
    context->words_in_this_subheading++; 

    if (syls > 1) 
      {
      if (kstring_ends_with_utf8 (clean_word, (UTF8*)"ed"))
        { 
        if ((kstring_strcmp_utf8 (context->last_word, (UTF8*)"is") == 0)
         || (kstring_strcmp_utf8 (context->last_word, (UTF8*)"was") == 0)
         || (kstring_strcmp_utf8 (context->last_word, (UTF8*)"being") == 0))
          {
          klog_debug (KLOG_CLASS, "Passive expression %S %S", 
            kstring_cstr (context->last_word), kstring_cstr(clean_word));
          context->passive_sentences++;
          }
        }
      }
    kstring_destroy (context->last_word);
    context->last_word = kstring_strdup (clean_word);
    }

  kstring_destroy (clean_word);
  KLOG_OUT
  }

/*============================================================================
  
  fkre_process 

  ==========================================================================*/
void fkre_process (FKREContext *context, KString *text)
  {
  size_t l = kstring_length (text);
  State state = STATE_START;
  
  KString *tag = kstring_new_empty();
  KString *word = kstring_new_empty();
  context->last_word = kstring_new_empty();

  for (int i = 0; i < l; i++)
    {
    int c = kstring_get (text, i);
    Type type = fkre_classify (context->html, c);
    if (state == STATE_TAG && type != TYPE_ENDTAG)
      {
      // If we've seen a start tag marker, then we don't pay any attention
      //  to the contents, until we get to the end tag, even if we
      //  hit end of file. Just buffer up the tag.
      kstring_append_char (tag, c);
      }
    else
      {
      switch (state * 1000 + type)
	{
        //
        // *** Events in START state *** 
        //
	case STATE_START * 1000 + TYPE_STARTTAG:
          klog_trace (KLOG_CLASS, "Start tag at pos %ld; new state TAG");
	  state = STATE_TAG;
	  break;

	case STATE_START * 1000 + TYPE_ENDTAG:
	  // This should never happen in well-formed HTML. We ignore
	  //   the end tag and carry on in start state
          klog_trace (KLOG_CLASS, "Unexpected end tag at pos %ld; "
                                     "new state START");
	  state = STATE_START;
	  break;

	case STATE_START * 1000 + TYPE_WHITE:
          klog_trace (KLOG_CLASS, "Whitespace at pos %ld; "
                                     "new state WHITE", i);
	  state = STATE_WHITE;
	  break;

	case STATE_START * 1000 + TYPE_TEXT:
          klog_trace (KLOG_CLASS, "Text at pos %ld; "
                                     "new state TEXT", i);
          kstring_append_char (word, c);
	  state = STATE_TEXT;
	  break;

        //
        // *** Events in TAG state *** 
        //
	case STATE_TAG * 1000 + TYPE_ENDTAG:
	  // Finished a tag. Go back to start state
          klog_trace (KLOG_CLASS, "End tag at pos %ld", i);
          fkre_do_tag (context, tag);
          kstring_clear (tag);
	  state = STATE_START;
	  break;

        //
        // *** Events in WHITE state *** 
        //
	case STATE_WHITE * 1000 + TYPE_WHITE:
          klog_trace (KLOG_CLASS, "Whitespace at pos %ld; "
                                     "stay in state WHITE", i);
	  state = STATE_WHITE;
	  break;

	case STATE_WHITE * 1000 + TYPE_STARTTAG:
          klog_trace (KLOG_CLASS, "Start tag at pos %ld; "
                                     "new state TAG", i);
	  state = STATE_TAG;
	  break;

	case STATE_WHITE * 1000 + TYPE_ENDTAG:
          klog_trace (KLOG_CLASS, "Unexpected end tag at pos %ld; "
                                     "new state START", i);
	  state = STATE_START;
	  break;

	case STATE_WHITE * 1000 + TYPE_TEXT:
          klog_trace (KLOG_CLASS, "Text at pos %ld; "
                                     "new state TEXT", i);
          kstring_append_char (word, c);
	  state = STATE_TEXT;
	  break;

        //
        // *** Events in TEXT state *** 
        //
	case STATE_TEXT * 1000 + TYPE_WHITE:
          klog_trace (KLOG_CLASS, "Whitespace at pos %ld; "
                                     "new state WHITE", i);
          fkre_do_word (context, word);
          kstring_clear (word);
	  state = STATE_WHITE;
	  break;

	case STATE_TEXT * 1000 + TYPE_STARTTAG:
          klog_trace (KLOG_CLASS, "Start tag at pos %ld; "
                                     "new state TAG");
          fkre_do_word (context, word);
          kstring_clear (word);
	  state = STATE_TAG;
	  break;

	case STATE_TEXT * 1000 + TYPE_ENDTAG:
	  // This should never happen in well-formed HTML. We ignore
	  //   the end tag and carry on in TEXT state
          klog_trace (KLOG_CLASS, "Unexpected end tag at pos %ld; "
                                     "stay in TEXT state");
	  state = STATE_TEXT;
	  break;

	case STATE_TEXT * 1000 + TYPE_TEXT:
          klog_trace (KLOG_CLASS, "Whitespace at pos %ld; "
                                     "stay in state WHITE", i);
          kstring_append_char (word, c);
	  state = STATE_TEXT;
	  break;

	default:
          // This should never happen
	  klog_error (KLOG_CLASS, 
	    "Internal error: char %d(%c) of type %d "
              "in state %d at position %ld", 
              c, (char)c, type, state, i);
	  exit (-1);
	}
      }
    }
  
  // End of file is essentially a subheading, so far as calculating
  //   the number of words per subheading
  if (context->html) fkre_got_subheading (context);

  kstring_destroy (word);
  kstring_destroy (tag);
  kstring_destroy (context->last_word);
  }

/*============================================================================
  
  fkre_calculate_score 

  ==========================================================================*/
BOOL fkre_calculate_score (FKREContext *context)
  {
  KLOG_IN
  BOOL ret = FALSE;
  if (context->sentences > 0 && context->words > 0)
    {
    double twords = context->words;
    double tsents = context->sentences;
    double tsylls = context->syllables;
    context->score = 206.835 
       - 1.015 * (twords / tsents) 
       - 84.6 * (tsylls / twords);
    ret = TRUE;
    }
  else
    {
    klog_warn (KLOG_CLASS, "Can't calculate FKRE score because some "
                              "divisors are zero");
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  fkre_show_usage 

  ==========================================================================*/
void fkre_show_usage (const char *argv0, FILE *f) 
  {
  fprintf (f, "Usage: %s [options] {filename}\n", argv0);
  fprintf (f, "    -t, --html     File is HTML\n");
  fprintf (f, "    -v, --version  Show version\n");
  }

/*============================================================================
  
  fkre_show_version

  ==========================================================================*/
void fkre_show_version (void) 
  {
  printf (NAME " version " VERSION "\n");
  printf ("Copyright (c)2020 Kevin Boone\n");
  printf 
    ("Distributed according to the terms of the GNU Public Licence, v3.0\n");
  }

/*============================================================================
  
  main 

  ==========================================================================*/
int main (int argc, char **argv)
  {
  int ret = 0;
  klog_init (KLOG_ERROR, NULL, NULL);
  klog_info (KLOG_CLASS, "Starting");

  BOOL show_version = FALSE;
  BOOL show_usage = FALSE;
  BOOL html = FALSE;

  int width = 80;
  int log_level = KLOG_ERROR;

  static struct option long_options[] =
    {
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {"log-level", required_argument, NULL, 'l'},
      {"html", no_argument, NULL, 't'},
      {"width", required_argument, NULL, 'w'},
      {0, 0, 0, 0}
    };

   int opt;
   ret = 0;
   while (ret == 0)
     {
     int option_index = 0;
     opt = getopt_long (argc, argv, "hvl:w:t",
     long_options, &option_index);

     if (opt == -1) break;

     switch (opt)
       {
       case 0:
         if (strcmp (long_options[option_index].name, "help") == 0)
           show_usage = TRUE;
         else if (strcmp (long_options[option_index].name, "html") == 0)
           html = TRUE;
         else if (strcmp (long_options[option_index].name, "version") == 0)
           show_version = TRUE;
         else if (strcmp (long_options[option_index].name, "log-level") == 0)
           log_level = atoi (optarg); 
         else if (strcmp (long_options[option_index].name, "width") == 0)
           width = atoi (optarg); 
         else
           ret = EINVAL; 
         break;
       case 'h': case '?': 
         show_usage = TRUE; break;
       case 'v': 
         show_version = TRUE; break;
       case 't': 
         html = TRUE; break;
       case 'l': 
           log_level = atoi (optarg); break;
       case 'w':
           width = atoi (optarg); break;
       default:
           ret = EINVAL;
       }
    }
  
  if (show_version)
    {
    fkre_show_version(); 
    ret = -1;
    }

  if (ret == 0 && show_usage)
    {
    fkre_show_usage (argv[0], stdout); 
    ret = -1;
    }

  if (width); // TODO
  klog_set_log_level (log_level);
  klog_set_handler (fkre_log_handler);

  if (ret == 0)
    {
    if (argc - optind != 1)
      {
      fkre_show_usage (argv[0], stderr); 
      ret = -1;
      }
    }
  
  if (ret == 0)
    {
    const char *filename = argv[optind];
    KPath *test = kpath_new_from_utf8 ((UTF8*)filename);

    KString *text = kpath_read_to_string (test);
    if (text)
      {
      FKREContext context;
      memset (&context, 0, sizeof (FKREContext));
      context.html = html; 
      fkre_process (&context, text); 
      printf ("Words: %d\n", context.words);
      printf ("Sentences: %d\n", context.sentences);
      printf ("Longest sentence: %d words\n", context.max_sentence_length);
      if (context.sentences > 0)
        printf ("Average sentence: %d words\n", 
          context.words / context.sentences);
      printf ("Syllables: %d\n", context.syllables);
      if (fkre_calculate_score (&context))
	{
	printf ("FK score: %.0f\n", context.score);
        printf ("FK rating: ");
        if (context.score > 90) printf ("very easy");
        else if (context.score > 80) printf ("easy");
        else if (context.score > 70) printf ("fairly easy");
        else if (context.score > 60) printf ("plain English");
        else if (context.score > 50) printf ("fairly difficult");
        else if (context.score > 30) printf ("difficult");
        else if (context.score > 10) printf ("very difficult");
        else printf ("extremely difficult");
        printf ("\n");
        printf ("Passive sentences: %d\n", context.passive_sentences);
        if (context.sentences > 0)
          printf ("Proportion of passive sentences: %.0f%%\n", 
            (double)context.passive_sentences / (double)context.sentences 
               * 100.0);
	}
      if (html)
        {
        printf ("Subheadings: %d\n", context.subheadings);
        printf ("Maximum words in a subheading: %d\n", 
           context.maximum_words_per_subheading);
        if (context.subheadings != 0)
          {
          printf ("Average words per subheading: %.0f\n", 
             (double)context.words / (double)context.subheadings);
          }
        }
      kstring_destroy (text);
      }
   else
      klog_error (KLOG_CLASS, "Can't read '%s': %s",  
        filename, strerror (errno)); 

    kpath_destroy (test);
    }

  klog_info (KLOG_CLASS, "Done");
  if (ret == -1) ret = 0;
  return ret;
  }


