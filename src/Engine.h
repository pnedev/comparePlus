/*
This file is part of Compare plugin for Notepad++
Copyright (C)2011 Jean-Sébastien Leroy (jean.sebastien.leroy@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ENGINE_H
#define ENGINE_H

#include "diff.h"
#include <string>
#include <assert.h>

using namespace std;

#define CLEANUP	1

/* Given a hash value and a new character, return a new hash value. */
#define HASH(h, c) ((c) + ROL (h, 7))

/* Rotate a value n bits to the left. */
#define UINT_BIT (sizeof (unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))

enum wordType
{
	SPACECHAR,ALPHANUMCHAR,OTHERCHAR
};

struct Word
{
	int line;
	int pos;
	int length;
	wordType type;
	string text;
	unsigned int hash;
};

struct chunk_info
{
	int *linePos;
	int *lineEndPos;
	int lineCount;
	int lineStart;
	struct varray<diff_change> *changes;
	struct varray<Word> *words;
	int changeCount;
	char *text;
	int count;
	//int *mappings;
	int *lineMappings;
};

struct blankLineList
{
	int line;
	int length;
	struct blankLineList *next;
};

int compareLines(unsigned int line1, unsigned int line2, void * /*context*/);
unsigned int getLineFromIndex(unsigned int *arr, int index, void * /*context*/);
int checkWords(diff_edit* e,chunk_info* chunk,chunk_info* otherChunk);
int compareWord(Word *word1, Word *word2, void * /*context*/);
Word *getWord(varray<Word> *words, int index, void * /*context*/);
bool compareWords(diff_edit* e1,diff_edit *e2,char** doc1,char** doc2, bool IncludeSpace);
int getWords(diff_edit* e, char** doc, chunk_info *chunk, bool IncludeSpace);
wordType getWordType(char letter);
int setDiffLines(diff_edit *e, diff_edit changes[], int *i, short op, int altLocation);
diff_edit *find_anchor(int line, varray<diff_edit> *ses, int sn, unsigned int *doc1, unsigned int *doc2, int *line2);
void find_moves(varray<diff_edit> *ses, int sn, unsigned int *doc1, unsigned int *doc2, bool DetectMove);
void shift_boundries(varray<diff_edit> *ses, int sn, unsigned int *doc1, unsigned int *doc2,
		int doc1Length, int doc2Length);
unsigned int *computeHashes(char** doc, int docLength, bool IncludeSpace);
void clearEdits(varray<diff_edit> *ses, int sn);
void clearEdit(diff_edit *e);
void cleanEmptyLines(blankLineList *line);

#endif // ENGINE_H
