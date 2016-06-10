/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include "Engine.h"
#include "diff.h"


struct chunk_info
{
	chunk_info(int line_offset, int line_count) :
		lineStart(line_offset), lineCount(line_count),
		linePos(line_count), lineEndPos(line_count), lineMappings(line_count, -1)
	{}

	unsigned int lineStart;
	unsigned int lineCount;

	std::vector<int> linePos;
	std::vector<int> lineEndPos;
	std::vector<int> lineMappings;

	varray_sh_ptr<diff_change> changes;
	unsigned int changeCount;

	std::vector<Word> words;

	char *text;
};


static wordType getWordType(char letter)
{
	switch (letter)
	{
		case ' ':
		case '\t':
		return SPACECHAR;

		case '\r':
		case '\n':
		return EOLCHAR;

		default:
			if (::IsCharAlphaNumericA(letter))
				return ALPHANUMCHAR;
		break;
	}

	return OTHERCHAR;
}


static int getWords(const DocLines_t& doc, chunk_info& chunk, bool IgnoreSpaces)
{
	unsigned int wordIndex = 0;

	for (unsigned int line = 0; line < chunk.lineCount; ++line)
	{
		std::string text("");
		wordType type = SPACECHAR;
		int len = 0;
		chunk.linePos[line] = wordIndex;
		int i = 0;
		unsigned int hash = 0;

		for (i = 0; doc[line + chunk.lineStart][i] != 0; ++i)
		{
			char l = doc[line + chunk.lineStart][i];
			wordType newType = getWordType(l);

			if (newType == type)
			{
				text += l;
				++len;
				hash = HASH(hash, l);
			}
			else
			{
				if (len > 0)
				{
					if (!IgnoreSpaces || type != SPACECHAR)
					{
						Word word;
						word.length = len;
						word.line = line;
						word.pos = i - len;
						word.type = type;
						word.hash = hash;
						chunk.words.push_back(word);
						++wordIndex;
					}
				}

				type = newType;
				text = l;
				len = 1;
				hash = HASH(0, l);
			}
		}

		if (len > 0)
		{
			if (!IgnoreSpaces || type != SPACECHAR)
			{
				Word word;
				word.length = len;
				word.line = line;
				word.pos = i - len;
				word.type = type;
				word.hash = hash;
				chunk.words.push_back(word);
				++wordIndex;
			}
		}

		chunk.lineEndPos[line] = wordIndex;
	}

	return wordIndex;
}


static int checkWords(diff_edit& e, chunk_info& chunk, chunk_info& otherChunk)
{
	int start = chunk.words[e.off].line;
	int end = chunk.words[e.off + e.len - 1].line;
	int line2 = chunk.lineMappings[start];
	int len = e.len;
	int off = e.off;

	// if the beginning is not at the start of the line, than its definitely a change;
	if (line2 == -1)
	{
		// if this line is not matched to another line, don't bother checking it
		if (start == end)
		{
			return chunk.changeCount;
		}
		else
		{
			e.len -= chunk.lineEndPos[start] - e.off;
			e.off = chunk.lineEndPos[start];
			++start;
		}
	}
	else if (e.off != (int)chunk.linePos[start])
	{
		diff_change& change = chunk.changes->get(chunk.changeCount++);
		diff_change& change2 = otherChunk.changes->get(otherChunk.changeCount++);

		change2.line = line2;
		change2.len = 0;
		change2.off = 0;
		change2.matchedLine = chunk.lineStart + start;

		change.off = chunk.words[e.off].pos;
		change.line = start;
		change.matchedLine = otherChunk.lineStart + line2;

		// multi-line change or single line change
		if (start != end)
		{
			len = chunk.lineEndPos[start] - e.off;
			e.off = chunk.lineEndPos[start];
			e.len -= len;
			Word& word = chunk.words[e.off + len - 1];
			change.len = (word.pos + word.length) - change.off;

			++start;
		}
		else
		{
			len = e.len;
			Word& word = chunk.words[e.off + len - 1];
			change.len = (word.pos + word.length) - change.off;

			return chunk.changeCount;
		}
	}

	// if a change spans more than one line, all the middle lines are just inserts or deletes
	while (start != end)
	{
		// potentially a inserted line
		e.off = chunk.lineEndPos[start];
		e.len -= (chunk.lineEndPos[start] - chunk.linePos[start]);
		++start;
	}

	line2 = chunk.lineMappings[start];

	// if the change does not go to the end of the line then its definitely a change
	if (line2 != -1 && (int)(e.off + e.len) < chunk.lineEndPos[start])
	{
		// TODO: recheck change because some of the diffs will be previous lines
		diff_change& change = chunk.changes->get(chunk.changeCount++);
		diff_change& change2 = otherChunk.changes->get(otherChunk.changeCount++);

		change2.line = line2;
		change2.matchedLine = chunk.lineStart + start;
		change2.len = 0;
		change2.off = 0;

		change.off = chunk.words[e.off].pos;
		len = e.len;
		Word& word = chunk.words[e.off + len - 1];
		change.len = (word.pos + word.length) - change.off;
		change.line = start;
		change.matchedLine = otherChunk.lineStart + line2;
	}

	e.off = off;
	e.len = len;

	return chunk.changeCount;
}


bool compareWords(diff_edit& e1, diff_edit& e2, const DocLines_t& doc1, const DocLines_t& doc2, bool IgnoreSpaces)
{
	unsigned int i, j;

	chunk_info chunk1(e1.off, e1.len);
	chunk_info chunk2(e2.off, e2.len);

	getWords(doc1, chunk1, IgnoreSpaces);
	getWords(doc2, chunk2, IgnoreSpaces);

	// Compare the two chunks
	std::vector<diff_edit> diff = DiffCalc<Word>(chunk1.words, chunk2.words)();

	chunk1.changes.reset(new varray<diff_change>);
	chunk2.changes.reset(new varray<diff_change>);

	std::vector<std::vector<int>> lineMappings1(chunk1.lineCount);

	for (i = 0; i < chunk1.lineCount; ++i)
		lineMappings1[i].resize(chunk2.lineCount, 0);

	// Use the MATCH results to synchronize line numbers
	// count how many are on each line, then select the line with the most matches
	const std::size_t diffSize = diff.size();

	int offset = 0;
	for (i = 0; i < diffSize; ++i)
	{
		diff_edit& e = diff[i];

		if (e.op == DIFF_DELETE)
		{
			offset -= e.len;
		}
		else if (e.op == DIFF_INSERT)
		{
			offset += e.len;
		}
		else
		{
			for (unsigned int index = e.off; index < (e.off + e.len); ++index)
			{
				Word *word1 = &chunk1.words[index];
				Word *word2 = &chunk2.words[index + offset];

				if (word1->type != SPACECHAR && word1->type != EOLCHAR)
				{
					int line1a = word1->line;
					int line2a = word2->line;
					lineMappings1[line1a][line2a] += word1->length;
				}
			}
		}
	}

	// go through each line, and select the line with the highest strength
	for (i = 0; i < chunk1.lineCount; ++i)
	{
		int line = -1;
		int max = 0;

		for (j = 0; j <chunk2.lineCount; ++j)
		{
			if (lineMappings1[i][j] > max && (e2.moves.empty() || e2.moves[j] == -1))
			{
				line = j;
				max = lineMappings1[i][j];
			}
		}

		// make sure that the line isn't already matched to another line,
		// and that enough of the line is matched to be significant
		const int size = doc1[e1.off + i].size();

		if (line != -1 && chunk2.lineMappings[line] == -1 && max > (size / 3) &&
				(e1.moves.empty() || e1.moves[i] == -1))
		{
			chunk1.lineMappings[i] = line;
			chunk2.lineMappings[line] = i;
		}
	}

	// find all the differences between the lines
	chunk1.changeCount = 0;
	chunk2.changeCount = 0;

	for (i = 0; i < diffSize; ++i)
	{
		diff_edit& e = diff[i];

		if (e.op == DIFF_DELETE)
		{
			// Differences for Doc 1
			checkWords(e, chunk1, chunk2);
		}
		else if (e.op == DIFF_INSERT)
		{
			// Differences for Doc2
			checkWords(e, chunk2, chunk1);
		}
	}

	e1.changeCount = chunk1.changeCount;
	e1.changes = chunk1.changes;
	e2.changeCount = chunk2.changeCount;
	e2.changes = chunk2.changes;

	return (chunk1.changeCount + chunk2.changeCount > 0);
}


// change the blocks of diffs to one diff per line.
// revert a "Changed" line to a insert or delete line if there are no changes
int setDiffLines(const diff_edit& e, std::vector<diff_edit>& changes, int* idx, short op, int altLocation)
{
	int index = *idx;
	int addedLines = 0;

	for (unsigned int j = 0; j < e.len; ++j)
	{
		changes[index].set = e.set;
		changes[index].len = 1;
		changes[index].op = e.op;
		changes[index].off = e.off + j;
		changes[index].changeCount = 0;

		// see if line is already marked as move
		if (!e.moves.empty() && e.moves[j] != -1)
		{
			changes[index].op = DIFF_MOVE;
			changes[index].matchedLine = e.moves[j];
			changes[index].altLocation = altLocation;
			++addedLines;
		}
		else
		{
			for (unsigned int k = 0; k < e.changeCount; ++k)
			{
				diff_change& change = e.changes->get(k);

				if (change.line == j)
				{
					changes[index].matchedLine = change.matchedLine;
					changes[index].altLocation = altLocation;

					if (altLocation != (int) change.matchedLine)
					{
						int diff = altLocation - change.matchedLine;
						altLocation = change.matchedLine;

						for (unsigned int i = 1; i <= j; i++)
						{
							if (changes[index - i].changes)
								break;

							if (op == DIFF_DELETE)
								changes[index - i].altLocation = change.matchedLine + diff;
							else
								changes[index - i].altLocation = change.matchedLine;
						}

						if (op == DIFF_INSERT)
							altLocation += diff;
					}

					if (!changes[index].changes)
						changes[index].changes.reset(new varray<diff_change>);

					diff_change& newChange = changes[index].changes->get(changes[index].changeCount++);

					newChange.len = change.len;
					newChange.off = change.off;
				}
			}

			if (!changes[index].changes)
			{
				changes[index].op = op;
				changes[index].altLocation = altLocation;
				++addedLines;
			}
			else
			{
				++altLocation;
			}
		}

		++index;
	}

	*idx = index;

	return addedLines;
}


// Move algorithm:
// scan for lines that are only in the other document once
// use one-to-one match as an anchor
// scan to see if the lines above and below anchor also match
static diff_edit* find_anchor(int line, std::vector<diff_edit>& diff,
		const unsigned int *hash1, const unsigned int *hash2, int *line2)
{
	diff_edit* insert = NULL;
	const std::size_t diffSize = diff.size();
	bool match = false;

	for (unsigned int i = 0; i < diffSize; ++i)
	{
		diff_edit& e = diff[i];

		if (e.op == DIFF_INSERT)
		{
			for (unsigned int j = 0; j < e.len; ++j)
			{
				if (hash1[line] == hash2[e.off + j])
				{
					if (match)
						return NULL;

					match = true;
					*line2 = j;
					insert = &e;
				}
			}
		}
	}

	if (!match || insert->moves[*line2] != -1)
		return NULL;

	match = false;

	for (unsigned int i = 0; i < diffSize; ++i)
	{
		const diff_edit& e = diff[i];

		if (e.op == DIFF_DELETE)
		{
			for (unsigned int j = 0; j < e.len; ++j)
			{
				if (hash1[line] == hash1[e.off + j])
				{
					if (match)
						return NULL;

					match = true;
				}
			}
		}
	}

	return insert;
}


void findMoves(std::vector<diff_edit>& diff, const unsigned int *hash1, const unsigned int *hash2)
{
	const std::size_t diffSize = diff.size();

	// initialize moves arrays
	for (unsigned int i = 0; i < diffSize; ++i)
	{
		diff_edit& e = diff[i];

		if (e.op != DIFF_MATCH)
			e.moves.resize(e.len, -1);
	}

	for (unsigned int i = 0; i < diffSize; ++i)
	{
		diff_edit& e = diff[i];

		if (e.op != DIFF_DELETE)
			continue;

		for (unsigned int j = 0; j < e.len; ++j)
		{
			if (e.moves[j] != -1)
				continue;

			int line2;
			diff_edit* match = find_anchor(e.off + j, diff, hash1, hash2, &line2);

			if (!match)
				continue;

			e.moves[j] = match->off + line2;
			match->moves[line2] = e.off + j;

			int d1 = j - 1;
			int d2 = line2 - 1;

			while (d1 >= 0 && d2 >= 0 && e.moves[d1] == -1 && match->moves[d2] == -1 &&
					hash1[e.off + d1] == hash2[match->off + d2])
			{
				e.moves[d1] = match->off + d2;
				match->moves[d2] = e.off + d1;
				--d1;
				--d2;
			}

			d1 = j + 1;
			d2 = line2 + 1;

			while (d1 < (int)e.len && d2 < (int)match->len && e.moves[d1] == -1 && match->moves[d2] == -1 &&
					hash1[e.off + d1] == hash2[match->off + d2])
			{
				e.moves[d1] = match->off + d2;
				match->moves[d2] = e.off + d1;
				++d1;
				++d2;
			}
		}
	}
}


// algorithm borrowed from WinMerge
// if the line after the delete is the same as the first line of the delete, shift down
// basically -abb is the same as -bba
// since most languages start with unique lines and end with repetitive lines (end, </node>, }, etc)
// we shift the differences down where its possible so the results will be cleaner
void shiftBoundries(std::vector<diff_edit>& diff,
		const unsigned int *hash1, const unsigned int *hash2, int doc1Length, int doc2Length)
{
	const std::size_t diffSize = diff.size();

	for (unsigned int i = 0; i < diffSize; ++i)
	{
		diff_edit& e1 = diff[i];

		int max1 = doc1Length;
		int max2 = doc2Length;
		int end2;

		if (e1.op != DIFF_MATCH)
		{
			for (unsigned int j = i + 1; j < diffSize; ++j)
			{
				diff_edit& e2 = diff[j];

				if (e2.op == e1.op)
				{
					max1 = e2.off;
					max2 = e2.off;
					break;
				}
			}
		}

		if (e1.op == DIFF_DELETE)
		{
			diff_edit& e2 = diff[i + 1];

			// if theres an insert after a delete, theres a potential match, so both blocks
			// need to be moved at the same time
			if (e2.op == DIFF_INSERT)
			{
				max2 = doc2Length;

				for (unsigned int j = i + 2; j < diffSize; ++j)
				{
					const diff_edit& e3 = diff[j];

					if (e2.op == e3.op)
					{
						max2 = e3.off;
						break;
					}
				}

				end2 = e2.off + e2.len;
				++i;

				int end1 = e1.off + e1.len;

				while (end1 < max1 && end2 < max2 && hash1[e1.off] == hash1[end1] && hash2[e2.off] == hash2[end2])
				{
					++end1;
					++end2;
					++(e1.off);
					++(e2.off);
				}
			}
			else
			{
				int end1 = e1.off + e1.len;

				while (end1 < max1 && hash1[e1.off] == hash1[end1])
				{
					++end1;
					++(e1.off);
				}
			}
		}
		else if (e1.op == DIFF_INSERT)
		{
			int end1 = e1.off + e1.len;

			while (end1 < max2 && hash2[e1.off] == hash2[end1])
			{
				++end1;
				++(e1.off);
			}
		}
	}
}


std::vector<unsigned int> computeHashes(const DocLines_t& doc, bool IgnoreSpaces)
{
	int docLength = doc.size();
	std::vector<unsigned int> hashes(docLength);

	for (int i = 0; i < docLength; ++i)
	{
		unsigned int hash = 0;

		for (int j = 0; doc[i][j] != 0; ++j)
		{
			if (doc[i][j] == ' ' || doc[i][j] == '\t')
			{
				if (!IgnoreSpaces)
					hash = HASH(hash, doc[i][j]);
			}
			else
			{
				hash = HASH(hash, doc[i][j]);
			}
		}

		hashes[i] = hash;
	}

	return hashes;
}
