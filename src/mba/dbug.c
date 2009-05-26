/* dbug - resolve symbols and print stack traces
 * Copyright (c) 2004 Michael B. Allen <mba2000 ioplex.com>
 *
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>

#if defined(_GNU_SOURCE) && defined(__i386__)

#include <execinfo.h>
#include <dlfcn.h>

#include "path.h"
#include "text.h"

int
dbug_stacktrace(void **buf, int off, int n)
{
	int i;
	void *_buf[16];

	if (n < 1 || n > 15) {
		n = 15;
	}
	off++;

	n = backtrace(_buf, n + off);

	for (i = off; i < n; i++) {
		buf[i - off] = _buf[i];
	}

	return i - off;
}
/* Put 'fname\0sname\0' into buf and return pointer to sname
 * Put '\0hexval\0' into buf and return pointer to hexval if
 * sym cannot be resolved
 */
unsigned char *
dbug_resolve_symbol(void *sym,
		unsigned char *buf,
		unsigned char *blim)
{
	Dl_info info;
	const char *name;

	if (dladdr(sym, &info) == 0) {
		*buf++ = '\0'; /* no filename */
		dsnprintf(buf, blim - buf, "%p", sym);
		return buf;
	}

	name = info.dli_fname;
	name = path_name((unsigned char *)name, (unsigned char *)name + 255, '/');
	buf += str_copy(name, name + 255, buf, blim, -1) + 1;
	name = info.dli_sname;
	str_copy(name, name + 255, buf, blim, -1);

	return buf;
}
int
dbug_sprint_stacktrace(unsigned char *str,
			unsigned char *slim,
			void **syms,
			int sn,
			const unsigned char *msg)
{
	unsigned char *start = str;
	const unsigned char *pad = ":  \n", *in = pad + 1, *nl = pad + 3;
	int si = 0;

	while (si < sn) {
		unsigned char buf[1024], *blim = buf + 1024, *sname;

		sname = dbug_resolve_symbol(syms[si++], buf, blim);
		*(sname - 1) = ':';
		str += str_copy(buf, blim, str, slim, -1);
		if (si == 1 && msg) {
			str += str_copy(pad, pad + 3, str, slim, 2);
			str += str_copy(msg, msg + 1024, str, slim, -1);
		}
		str += str_copy(nl, nl + 2, str, slim, 1);
		if (si == sn) {
			break;
		}
		str += str_copy(in, in + 3, str, slim, 2);
	}

	return str - start;
}
int
dbug_fprint_stacktrace(FILE *stream, int off, int n, const char *msg)
{
	void *syms[16];
	unsigned char str[1024];

	n = dbug_stacktrace(syms, off, n + 1);
	if (n && (n = dbug_sprint_stacktrace(str, str + 1024, syms + 1, n - 1, msg)) == -1) {
		return -1;
	}
	fwrite(str, 1, n, stream);
	fflush(stream);

	return 0;
}

#else /* backtraces not supported - emit stub implementation */

int
dbug_stacktrace(void **buf, int off, int n)
{
	(void)buf;
	(void)off;
	(void)n;
	return 0;
}
unsigned char *
dbug_resolve_symbol(void *sym, unsigned char *buf, unsigned char *blim)
{
	(void)sym;
	(void)buf;
	(void)blim;
	return NULL;
}
int
dbug_sprint_stacktrace(unsigned char *str,
			unsigned char *slim,
			void **syms,
			int sn,
			const unsigned char *msg)
{
	(void)str;
	(void)slim;
	(void)syms;
	(void)sn;
	(void)msg;
	return 0;
}
int
dbug_fprint_stacktrace(FILE *stream, int off, int n, const char *msg)
{
	(void)stream;
	(void)off;
	(void)n;
	(void)msg;
	return 0;
}

#endif
