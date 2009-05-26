#ifndef MBA_MSGNO_H
#define MBA_MSGNO_H

/* msgno - managing error codes and associated messages across
 * separate C libraries
 */

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

#define STR0(s) #s
#define STR1(s) STR0(s)
#define LINE_STRING STR1(__LINE__)
#if (__STDC_VERSION__ >= 199901L) || defined(__GNUC__)
#define LOC0 __FILE__ ":" LINE_STRING ":"
#define LOC1 __func__
#else
#define LOC0 __FILE__ ":"
#define LOC1 LINE_STRING
#endif

#define MMSG msgno_loc0(LOC0, LOC1); msgno_mmsg0
#define MMNO msgno_loc0(LOC0, LOC1); msgno_mmno0
#define MMNF msgno_loc0(LOC0, LOC1); msgno_mmnf0
#define PMSG msgno_loc0("!" LOC0, LOC1); msgno_amsg0
#define PMNO msgno_loc0("!" LOC0, LOC1); msgno_amno0
#define PMNF msgno_loc0("!" LOC0, LOC1); msgno_amnf0
#define AMSG msgno_loc0(LOC0, LOC1); msgno_amsg0
#define AMNO msgno_loc0(LOC0, LOC1); msgno_amno0
#define AMNF msgno_loc0(LOC0, LOC1); msgno_amnf0

#define MCLR (msgno_buf[msgno_buf_idx = 0] = 0)

#define NULL_POINTER_ERR msgno_builtin_codes[0].msgno

struct msgno_entry {
	int msgno;
	const char *msg;
};

LIBMBA_API struct msgno_entry msgno_builtin_codes[];
LIBMBA_API char msgno_buf[];
LIBMBA_API int msgno_buf_idx;
LIBMBA_API int msgno_append(const char *src, int n);
LIBMBA_API int msgno_loc0(const char *loc0, const char *loc1);
LIBMBA_API int msgno_mmsg0(const char *fmt, ...);
LIBMBA_API int msgno_mmno0(int msgno);
LIBMBA_API int msgno_mmnf0(int msgno, const char *fmt, ...);
LIBMBA_API int msgno_amsg0(const char *fmt, ...);
LIBMBA_API int msgno_amno0(int msgno);
LIBMBA_API int msgno_amnf0(int msgno, const char *fmt, ...);
LIBMBA_API int msgno_hdlr_stderr(const char *fmt, ...);

LIBMBA_API int (*msgno_hdlr)(const char *fmt, ...);
LIBMBA_API int msgno_add_codes(struct msgno_entry *lst);
LIBMBA_API const char *msgno_msg(int msgno);

#ifdef __cplusplus
}
#endif

#endif /* MBA_MSGNO_H */
