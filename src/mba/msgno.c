/* msgno - managing error codes and associated messages across
 * separate C libraries
 * Copyright (c) 2001 Michael B. Allen <mba2000 ioplex.com>
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "msgno.h"

#ifndef MSGNO_NUM_LISTS
#define MSGNO_NUM_LISTS 16
#endif
#ifndef MSGNO_BUFSIZ
#define MSGNO_BUFSIZ 1024
#endif

struct msgno_entry msgno_builtin_codes[2] = {
	{ (1 << 16), "A parameter was NULL" },
	{ 0, NULL }
};

static struct tbl_entry {
	struct msgno_entry *list;
	unsigned int num_msgs;
} list_tbl[MSGNO_NUM_LISTS] = {
	{ msgno_builtin_codes, 1 }
};

static unsigned int next_tbl_idx = 1;

int msgno_buf_idx = 0;
char msgno_buf[MSGNO_BUFSIZ] = { 0 };

int
msgno_add_codes(struct msgno_entry *list)
{
	struct tbl_entry *te;
	int next_msgno = 0, hi_bits;

	if (list == NULL || list->msg == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (next_tbl_idx == MSGNO_NUM_LISTS) {
		errno = ERANGE;
		return -1;
	}

	for (te = list_tbl + 1; te->list; te++) {
		if (te->list == list) {
			return 0; /* already in list_tbl */
		}
	}

	hi_bits = (next_tbl_idx + 1) << 16;
	te->list = list;
	while (list->msg) {
		if ((list->msgno & 0xFFFF0000)) {
			te->list = NULL;
			errno = ERANGE;
			return -1;
		}
		if (list->msgno == 0) {
			list->msgno = hi_bits | next_msgno++;
		} else if (list->msgno >= next_msgno) {
			next_msgno = list->msgno + 1;
			list->msgno = hi_bits | list->msgno;
		} else {
			te->list = NULL;
			errno = ERANGE;
			return -1;
		}
		te->num_msgs++;
		list++;
	}
	next_tbl_idx++;

	return 0;
}

const char *
msgno_msg(int msgno)
{
	struct tbl_entry *te;
	unsigned int i;

	i = msgno >> 16;
	if (i == 0) {
		return strerror(msgno);
	} else if (i >= MSGNO_NUM_LISTS || (te = list_tbl + (i - 1)) == NULL) {
		return "No such msgno list";
	}

	for (i = 0; i < te->num_msgs; i++) {
		if (te->list[i].msgno == msgno) {
			return te->list[i].msg;
		}
	}

	return "No such message in msgno list";
}

int
msgno_hdlr_stderr(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	fflush(stderr);
	return 0;
}

int (*msgno_hdlr)(const char *fmt, ...) = msgno_hdlr_stderr;

int
msgno_append(const char *src, int n)
{
	unsigned char *dst = (unsigned char *)msgno_buf + msgno_buf_idx;
	unsigned char *dlim = (unsigned char *)msgno_buf + MSGNO_BUFSIZ;
	unsigned char *start = dst;

	if (src == NULL || n < 1 || dst >= dlim) {
		return 0;
	}
	while (n-- && *src) {
		*dst++ = *src++;
		if (dst == dlim) {
			dst--;
			break;
		}
	}
	*dst = '\0';
	msgno_buf_idx += dst - start;

	return dst - start;
}
int
msgno_vsprintf(const char *fmt, va_list ap)
{
	size_t size = MSGNO_BUFSIZ - msgno_buf_idx;
	int n;

#if (__STDC_VERSION__ >= 199901L)
	if ((n = vsnprintf(msgno_buf + msgno_buf_idx, size, fmt, ap)) < 0 ||
#else
	if ((n = vsprintf(msgno_buf + msgno_buf_idx, fmt, ap)) < 0 ||
#endif
				(size_t)n >= size || msgno_buf_idx > MSGNO_BUFSIZ) {
		*msgno_buf = '\0';
		msgno_buf_idx = 0;
		n = msgno_append("vsnprintf error", 15);
	}
	msgno_buf_idx += n;

	return n;
}
int
msgno_loc0(const char *loc0, const char *loc1)
{
	if (*loc0 == '!') {
		loc0++;
		*msgno_buf = '\0';
		msgno_buf_idx = 0;
	} else if (*msgno_buf) {
		msgno_buf[msgno_buf_idx++] = ' ';
		msgno_buf[msgno_buf_idx++] = ' ';
	}
	return msgno_append(loc0, 128) + msgno_append(loc1, 128) + msgno_append(": ", 2);
}
int
msgno_mmsg0(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	msgno_vsprintf(fmt, ap);
	msgno_hdlr("%s", msgno_buf);
	*msgno_buf = '\0';
	msgno_buf_idx = 0;
	va_end(ap);

	return 0;
}
int
msgno_mmno0(int msgno)
{
	msgno_append(msgno_msg(msgno), 255);
	msgno_hdlr(msgno_buf);
	*msgno_buf = '\0';
	msgno_buf_idx = 0;

	return 0;
}
int
msgno_mmnf0(int msgno, const char *fmt, ...)
{
	va_list ap;

	msgno_append(msgno_msg(msgno), 255);
	va_start(ap, fmt);
	msgno_vsprintf(fmt, ap);
	msgno_hdlr("%s", msgno_buf);
	*msgno_buf = '\0';
	msgno_buf_idx = 0;
	va_end(ap);

	return 0;
}
int
msgno_amsg0(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	msgno_vsprintf(fmt, ap);
	msgno_buf[msgno_buf_idx++] = '\n';
	va_end(ap);

	return 0;
}
int
msgno_amno0(int msgno)
{
	msgno_append(msgno_msg(msgno), 255);
	msgno_buf[msgno_buf_idx++] = '\n';
	return 0;
}
int
msgno_amnf0(int msgno, const char *fmt, ...)
{
	va_list ap;

	msgno_append(msgno_msg(msgno), 255);
	va_start(ap, fmt);
	msgno_vsprintf(fmt, ap);
	msgno_buf[msgno_buf_idx++] = '\n';
	va_end(ap);

	return 0;
}

