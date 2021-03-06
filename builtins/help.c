/* help.c, created from help.def. */
#line 22 "./help.def"

#line 45 "./help.def"

#include <config.h>

#if defined (HELP_BUILTIN)
#include <stdio.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <errno.h>

#include <filecntl.h>

#include "../bashintl.h"

#include "../shell.h"
#include "../builtins.h"
#include "../pathexp.h"
#include "common.h"
#include "bashgetopt.h"

#include <glob/strmatch.h>
#include <glob/glob.h>

#ifndef errno
extern int errno;
#endif

extern const char * const bash_copyright;
extern const char * const bash_license;

static void show_builtin_command_help __P((void));
static int open_helpfile __P((char *));
static void show_desc __P((char *, int));
static void show_manpage __P((char *, int));
static void show_longdoc __P((int));

/* Print out a list of the known functions in the shell, and what they do.
   If LIST is supplied, print out the list which matches for each pattern
   specified. */
int
help_builtin (list)
     WORD_LIST *list;
{
  register int i;
  char *pattern, *name;
  int plen, match_found, sflag, dflag, mflag;

  dflag = sflag = mflag = 0;
  reset_internal_getopt ();
  while ((i = internal_getopt (list, "dms")) != -1)
    {
      switch (i)
	{
	case 'd':
	  dflag = 1;
	  break;
	case 'm':
	  mflag = 1;
	  break;
	case 's':
	  sflag = 1;
	  break;
	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }
  list = loptend;

  if (list == 0)
    {
      show_shell_version (0);
      show_builtin_command_help ();
      return (EXECUTION_SUCCESS);
    }

  /* We should consider making `help bash' do something. */

  if (glob_pattern_p (list->word->word))
    {
      printf (ngettext ("Shell commands matching keyword `", "Shell commands matching keywords `", (list->next ? 2 : 1)));
      print_word_list (list, ", ");
      printf ("'\n\n");
    }

  for (match_found = 0, pattern = ""; list; list = list->next)
    {
      pattern = list->word->word;
      plen = strlen (pattern);

      for (i = 0; name = shell_builtins[i].name; i++)
	{
	  QUIT;
	  if ((strncmp (pattern, name, plen) == 0) ||
	      (strmatch (pattern, name, FNMATCH_EXTFLAG) != FNM_NOMATCH))
	    {
	      match_found++;
	      if (dflag)
		{
		  show_desc (name, i);
		  continue;
		}
	      else if (mflag)
		{
		  show_manpage (name, i);
		  continue;
		}

	      printf ("%s: %s\n", name, shell_builtins[i].short_doc);

	      if (sflag == 0)
		show_longdoc (i);
	    }
	}
    }

  if (match_found == 0)
    {
      builtin_error (_("no help topics match `%s'.  Try `help help' or `man -k %s' or `info %s'."), pattern, pattern, pattern);
      return (EXECUTION_FAILURE);
    }

  fflush (stdout);
  return (EXECUTION_SUCCESS);
}

static int
open_helpfile (name)
     char *name;
{
  int fd;

  fd = open (name, O_RDONLY);
  if (fd == -1)
    {
      builtin_error (_("%s: cannot open: %s"), name, strerror (errno));
      return -1;
    }
  return fd;
}

/* By convention, enforced by mkbuiltins.c, if separate help files are being
   used, the long_doc array contains one string -- the full pathname of the
   help file for this builtin.  */
static void
show_longdoc (i)
     int i;
{
  register int j;
  char * const *doc;
  int fd;

  doc = shell_builtins[i].long_doc;

  if (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL)
    {
      fd = open_helpfile (doc[0]);
      if (fd < 0)
	return;
      zcatfd (fd, 1, doc[0]);
      close (fd);
    }
  else
    for (j = 0; doc[j]; j++)
      printf ("%*s%s\n", BASE_INDENT, " ", _(doc[j]));
}

static void
show_desc (name, i)
     char *name;
     int i;
{
  register int j;
  char **doc, *line;
  int fd, usefile;

  doc = (char **)shell_builtins[i].long_doc;

  usefile = (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL);
  if (usefile)
    {
      fd = open_helpfile (doc[0]);
      if (fd < 0)
	return;
      zmapfd (fd, &line, doc[0]);
      close (fd);
    }
  else
    line = doc ? doc[0] : (char *)NULL;

  printf ("%s - ", name);
  for (j = 0; line && line[j]; j++)
    {
      putchar (line[j]);
      if (line[j] == '\n')
	break;
    }
  
  fflush (stdout);

  if (usefile)
    free (line);
}

/* Print builtin help in pseudo-manpage format. */
static void
show_manpage (name, i)
     char *name;
     int i;
{
  register int j;
  char **doc, *line;
  int fd, usefile;

  doc = (char **)shell_builtins[i].long_doc;

  usefile = (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL);
  if (usefile)
    {
      fd = open_helpfile (doc[0]);
      if (fd < 0)
	return;
      zmapfd (fd, &line, doc[0]);
      close (fd);
    }
  else
    line = doc ? _(doc[0]) : (char *)NULL;

  /* NAME */
  printf ("NAME\n");
  printf ("%*s%s - ", BASE_INDENT, " ", name);
  for (j = 0; line && line[j]; j++)
    {
      putchar (line[j]);
      if (line[j] == '\n')
	break;
    }
  printf ("\n");

  /* SYNOPSIS */
  printf ("SYNOPSIS\n");
  printf ("%*s%s\n\n", BASE_INDENT, " ", shell_builtins[i].short_doc);

  /* DESCRIPTION */
  printf ("DESCRIPTION\n");
  if (usefile == 0)
    {
      for (j = 0; doc[j]; j++)
        printf ("%*s%s\n", BASE_INDENT, " ", _(doc[j]));
    }
  else
    {
      for (j = 0; line && line[j]; j++)
	{
	  putchar (line[j]);
	  if (line[j] == '\n')
	    printf ("%*s", BASE_INDENT, " ");
	}
    }
  putchar ('\n');

  /* SEE ALSO */
  printf ("SEE ALSO\n");
  printf ("%*sbash(1)\n\n", BASE_INDENT, " ");

  /* IMPLEMENTATION */
  printf ("IMPLEMENTATION\n");
  printf ("%*s", BASE_INDENT, " ");
  show_shell_version (0);
  printf ("%*s", BASE_INDENT, " ");
  printf ("%s\n", _(bash_copyright));
  printf ("%*s", BASE_INDENT, " ");
  printf ("%s\n", _(bash_license));

  fflush (stdout);
  if (usefile)
    free (line);
}

static void
show_builtin_command_help ()
{
  int i, j;
  int height, width;
  char *t, blurb[128];

  printf (
_("These shell commands are defined internally.  Type `help' to see this list.\n\
Type `help name' to find out more about the function `name'.\n\
Use `info bash' to find out more about the shell in general.\n\
Use `man -k' or `info' to find out more about commands not in this list.\n\
\n\
A star (*) next to a name means that the command is disabled.\n\
\n"));

  t = get_string_value ("COLUMNS");
  width = (t && *t) ? atoi (t) : 80;
  if (width <= 0)
    width = 80;

  width /= 2;
  if (width > sizeof (blurb))
    width = sizeof (blurb);
  if (width <= 3)
    width = 40;
  height = (num_shell_builtins + 1) / 2;	/* number of rows */

  for (i = 0; i < height; i++)
    {
      QUIT;

      /* first column */
      blurb[0] = (shell_builtins[i].flags & BUILTIN_ENABLED) ? ' ' : '*';
      strncpy (blurb + 1, shell_builtins[i].short_doc, width - 2);
      blurb[width - 2] = '>';		/* indicate truncation */
      blurb[width - 1] = '\0';
      printf ("%s", blurb);
      if (((i << 1) >= num_shell_builtins) || (i+height >= num_shell_builtins))
	{
	  printf ("\n");
	  break;
	}

      /* two spaces */
      for (j = strlen (blurb); j < width; j++)
        putc (' ', stdout);

      /* second column */
      blurb[0] = (shell_builtins[i+height].flags & BUILTIN_ENABLED) ? ' ' : '*';
      strncpy (blurb + 1, shell_builtins[i+height].short_doc, width - 3);
      blurb[width - 3] = '>';		/* indicate truncation */
      blurb[width - 2] = '\0';
      printf ("%s\n", blurb);
    }
}
#endif /* HELP_BUILTIN */
