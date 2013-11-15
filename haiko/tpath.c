#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


static char const dirsep = '/';
static char const * userdir = ".haiko";

#ifdef DATADIR
static char const * sharedir = DATADIR;
#else
static char const * sharedir = "/usr/local/share/haiko";
#endif


/* If fname begins with a slash, use only the absolute path. If it
   contains a slash somewhere else, use only the relative
   path. Otherwise search first the CWD, then $HAIKO_PATH if set, then
   $HOME/userdir if HOME is set, then sharedir, then give up. */


/**
   \return 1 if fname begins with dirsep, 0 if not, and -1 on error
   (NULL or empty fname)
*/
static int is_absolute(char const * fname)
{
  if (NULL == fname)
    return -1;
  if (1 > strlen(fname))
    return -1;
  if (dirsep == fname[0])
    return 1;
  return 0;
}


/**
   \return 1 if fname contains a dirsep, 0 if not, and -1 on error
   (NULL or empty fname)
*/
static int has_dirsep(char const * fname)
{
  size_t len, ii;
  if (NULL == fname)
    return -1;
  len = strlen(fname);
  if (1 > len)
    return -1;
  for (ii = 0; ii < len; ii++)
    if (dirsep == fname[ii])
      return 1;
  return 0;
}


/**
   Concatenates two paths lhs+rhs, withOUT adding dirsep between them,
   then calls realpath() to canonicalize the result.
   
   \return 0 on success, -1 on argument error (NULL or empty lhs or
   rhs), -2 if realpath() fails (in which case an error message is
   written to stderr).
*/
static int path_concat(/** buf is assumed to be char[PATH_MAX] and
			   overwritten */
		       char * buf, char const * lhs, char const * rhs,
		       int verbose)
{
  char lbuf[PATH_MAX];
  size_t lhslen, rhslen;
  char * cur;
  
  if (verbose)
    printf("  path_concat(buf, \"%s\", \"%s\")\n", lhs, rhs);
  
  if ((NULL == lhs) || (NULL == rhs)) {
    if (verbose)
      printf("  path_concat(): lhs or rhs is NULL\n");
    return -1;
  }
  lhslen = strlen(lhs);
  rhslen = strlen(rhs);
  if ((1 > lhslen) || (1 > rhslen)) {
    if (verbose)
      printf("  path_concat(): lhs or rhs is empty\n");
    return -1;
  }
  if (PATH_MAX - 1 <= lhslen + rhslen) { /* -1 because we add a dirsep */
    if (verbose)
      printf("  path_concat(): lhs+rhs is too long\n");
    return -1;
  }
  
  cur = stpcpy(lbuf, lhs);
  *cur = dirsep;
  strcpy(cur + 1, rhs);
  
  if (verbose)
    printf("  path_concat(): calling realpath(\"%s\", buf)\n", lbuf);
  
  if (NULL == realpath(lbuf, buf)) {
    if (verbose)
      printf("  path_concat(): %s: %s\n", buf, strerror(errno));
/*     else */
/*       fprintf(stderr, "  path_concat(): realpath(%s) failed\n    %s: %s\n", */
/* 	      lbuf, buf, strerror(errno)); */
    return -2;
  }
  return 0;
}


static int file_exists(char const * path)
{
  FILE * ff = fopen(path, "r");
  if (NULL == ff)
    return 0;
  fclose(ff);
  return 1;
}


int main(int argc, char ** argv)
{
  char buf[PATH_MAX];
  char * home = getenv("HOME");
  char * haiko_path = getenv("HAIKO_PATH");
  char userdir_path[PATH_MAX];
  char sharedir_path[PATH_MAX];
  int ii;
  int verbose = 0;
  
  if (NULL == home) {
    printf("no home directory\n");
    userdir_path[0] = '\0';
  }
  else {
    if (verbose)
      printf("abspath of userdir \"%s\" + \"%s\"\n", home, userdir);
    switch (path_concat(userdir_path, home, userdir, verbose)) {
    case -1:
      printf("  ERROR in path_concat()\n");
      userdir_path[0] = '\0';
      break;
    case -2:
      printf("  realpath() failed\n");
      userdir_path[0] = '\0';
      break;
    case 0:
      if (verbose)
	printf("  RESOLVED \"%s\"\n", userdir_path);
      break;
    default:
      printf("  ERROR unhandled path_concat() retval\n");
      userdir_path[0] = '\0';
    }
    if ('\0' != userdir_path[0])
      printf("userdir_path \"%s\"\n", userdir_path);
  }
  
  if (NULL == realpath(sharedir, sharedir_path)) {
    printf("invalid sharedir \"%s\"\n  %s: %s\n",
	   sharedir, sharedir_path, strerror(errno));
    sharedir_path[0] = '\0';
  }
  else
    printf("sharedir_path \"%s\"\n", sharedir_path);
  
  for (ii = 1; ii < argc; ii++) {
    printf("checking filename \"%s\"\n", argv[ii]);
    switch (is_absolute(argv[ii])) {
    case -1:
      printf("  ERROR in is_absolute()\n");
      continue;
    case 0:
      /* not absolute */
      break;
    case 1:
      printf("  resolved absolute path\n");
      if (file_exists(argv[ii])) {
	printf("  FOUND absolute path\n");
	continue;
      }
      printf("  absolute path is not a (valid) file\n");
      break;
    default:
      printf("  ERROR unhandled is_absolute() retval\n");
    }
    
    switch (has_dirsep(argv[ii])) {
    case -1:
      printf("  ERROR in has_dirsep()\n");
      continue;
    case 0:
      /* no dirsep in fname */
      break;
    case 1:
      printf("  RESOLVED relative path\n");
      continue;
    default:
      printf("  ERROR unhandled has_dirsep() retval\n");
    }
    
    if (NULL == haiko_path)
      printf("  no HAIKO_PATH\n");
    else {
      printf("  HAIKO_PATH + fname = \"%s%s\"\n", haiko_path, argv[ii]);
      switch (path_concat(buf, haiko_path, argv[ii], verbose)) {
      case -1:
	printf("    ERROR in path_concat()\n");
	break;
      case -2:
	printf("    realpath() failed\n");
	break;
      case 0:
	printf("    resolved file in HAIKO_PATH: \"%s\"\n", buf);
	if (file_exists(buf)) {
	  printf("    FOUND in HAIKO_PATH\n");
	  continue;
	}
	printf("    but it is not a (valid) file\n");
	break;
      default:
	printf("    ERROR unhandled path_concat() retval\n");
      }
    }
    
    if ('\0' == userdir_path[0])
      printf("  no userdir_path\n");
    else {
      printf("  userdir_path + fname = \"%s%s\"\n", userdir_path, argv[ii]);
      switch (path_concat(buf, userdir_path, argv[ii], verbose)) {
      case -1:
	printf("    ERROR in path_concat()\n");
	break;
      case -2:
	printf("    realpath() failed\n");
	break;
      case 0:
	printf("    resolved file in userdir: \"%s\"\n", buf);
	if (file_exists(buf)) {
	  printf("    FOUND in userdir\n");
	  continue;
	}
	printf("    but it is not a (valid) file\n");
	break;
      default:
	printf("    ERROR unhandled path_concat() retval\n");
      }
    }
    
    if ('\0' == sharedir_path[0])
      printf("  no sharedir_path\n");
    else {
      printf("  sharedir_path + fname = \"%s%s\"\n", sharedir_path, argv[ii]);
      switch (path_concat(buf, sharedir_path, argv[ii], verbose)) {
      case -1:
	printf("    ERROR in path_concat()\n");
	break;
      case -2:
	printf("    realpath() failed\n");
	break;
      case 0:
	printf("    resolved file in sharedir: \"%s\"\n", buf);
	if (file_exists(buf)) {
	  printf("    FOUND in sharedir\n");
	  continue;
	}
	printf("    but it is not a (valid) file\n");
	break;
      default:
	printf("    ERROR unhandled path_concat() retval\n");
      }
    }
    
    printf("  SORRY, could not resolve file \"%s\"\n", argv[ii]);
  }
  
  return 0;
}
