/* myftw.h -- ftw() using lstat() instead of stat() */

#ifndef _H_MYFTW_
#define _H_MYFTW_

#include <sys/stat.h>

/* The FLAG argument to the user function passed to ftw.  */
#define MYFTW_F		0		/* Regular file.  */
#define MYFTW_D		1		/* Directory.  */
#define MYFTW_DNR	2		/* Unreadable directory.  */
#define MYFTW_NS	3		/* Unstatable file.  */

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*myftwFunc) (void *fl, char *name, struct stat *statp);

int myftw (const char *dir,
	   int descriptors,
	   myftwFunc func,
	   void *fl);

#ifdef __cplusplus
}
#endif

#endif /* _H_MYFTW_ */
