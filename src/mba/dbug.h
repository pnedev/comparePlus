#ifndef MBA_DBUG_H
#define MBA_DBUG_H

/* dbug - resolve symbols and print stack traces w/ x86 GNUC
 */

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBMBA_API
#ifdef WIN32
# ifdef LIBMBA_EXPORTS
#  define LIBMBA_API  __declspec(dllexport)
# else /* LIBMBA_EXPORTS */
#  define LIBMBA_API  __declspec(dllimport)
# endif /* LIBMBA_EXPORTS */
#else /* WIN32 */
# define LIBMBA_API extern
#endif /* WIN32 */
#endif /* LIBMBA_API */

LIBMBA_API int dbug_stacktrace(void **buf, int off, int n);
LIBMBA_API unsigned char *dbug_resolve_symbol(void *sym, unsigned char *buf, unsigned char *blim);
LIBMBA_API int dbug_sprint_stacktrace(unsigned char *str,
			unsigned char *slim,
			void **syms,
			int sn,
			const unsigned char *msg);
LIBMBA_API int dbug_fprint_stacktrace(FILE *stream, int off, int n, const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* MBA_DBUG_H */
