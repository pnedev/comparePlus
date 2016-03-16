/* diff - compute a shortest edit script (SES) given two sequences
 */

#pragma once

#include <varray.h>


// CProgess callback declaration
bool CProgressUpdate(int mid);

typedef const void *(*idx_fn)(const void *s, int idx, void *context);
typedef int (*cmp_fn)(const void *object1, const void *object2, void *context);

typedef enum {
	DIFF_MATCH = 1,
	DIFF_DELETE,
	DIFF_INSERT,
	DIFF_CHANGE1,
	DIFF_CHANGE2,
	DIFF_MOVE
} diff_op;

struct diff_change {
	int off;
	int len;
	int line;
	int matchedLine;
};

struct diff_edit {
	short op;
	int off; /* off into s1 if MATCH or DELETE but s2 if INSERT */
	int len;

	// added lines for Notepad++ use
	int set;
	int matchedLine;
	int altLocation;
	struct varray<diff_change> *changes;
	int changeCount;
	int* moves;
};

/* consider alternate behavior for each NULL parameter
 */
int diff(const void *a, int aoff, int n,
		const void *b, int boff, int m,
		idx_fn idx, cmp_fn cmp, void *context, int dmax,
		struct varray<diff_edit> *ses, int *sn,
		struct varray<int> *buf);
