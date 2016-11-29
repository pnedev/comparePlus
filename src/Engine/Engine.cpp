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
#include <climits>
#include "Engine.h"


// Rotate a value n bits to the left
#define UINT_BIT (sizeof(unsigned) * CHAR_BIT)
#define ROL(v, n) ((v) << (n) | (v) >> (UINT_BIT - (n)))

// Given a hash value and a new character, return a new hash value
#define HASH(h, c) ((c) + ROL(h, 7))


namespace {

enum diff_mark {
	DELETED_MARK = 0,
	ADDED_MARK,
	MOVED_MARK,
	CHANGED_MARK,
	END_MARK
};


const Marker_t lineMark[END_MARK] = {
	MARKER_REMOVED_LINE,
	MARKER_ADDED_LINE,
	MARKER_MOVED_LINE,
	MARKER_CHANGED_LINE
};


const Marker_t symbolMark[END_MARK] = {
	MARKER_REMOVED_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_MOVED_SYMBOL,
	MARKER_CHANGED_SYMBOL
};


struct chunk_info
{
	chunk_info(int line_offset, int line_count) :
		lineStart(line_offset), lineCount(line_count),
		lineStartWordIdx(line_count), lineEndWordIdx(line_count), lineMappings(line_count, -1)
	{}

	int lineStart;
	int lineCount;

	std::vector<int> lineStartWordIdx;
	std::vector<int> lineEndWordIdx;
	std::vector<int> lineMappings;

	std::vector<Word> words;
};


std::vector<unsigned int> computeLineHashes(HWND view, const UserSettings& settings)
{
	int docLines = ::SendMessage(view, SCI_GETLENGTH, 0, 0);

	if (docLines)
		docLines = ::SendMessage(view, SCI_GETLINECOUNT, 0, 0);

	std::vector<unsigned int> lineHashes(docLines);

	for (int lineNum = 0; lineNum < docLines; ++lineNum)
	{
		const int lineStart = ::SendMessage(view, SCI_POSITIONFROMLINE, lineNum, 0);
		const int lineEnd = ::SendMessage(view, SCI_GETLINEENDPOSITION, lineNum, 0);

		unsigned int hash = 0;

		if (lineEnd - lineStart)
		{
			const std::vector<char> line = getText(view, lineStart, lineEnd);
			const int lineLen = line.size() - 1;

			for (int i = 0; i < lineLen; ++i)
			{
				if (settings.IgnoreSpaces && (line[i] == ' ' || line[i] == '\t'))
					continue;

				hash = HASH(hash, line[i]);
			}
		}

		lineHashes[lineNum] = hash;
	}

	return lineHashes;
}


charType getCharType(char letter)
{
	switch (letter)
	{
		case ' ':
		case '\t':
		return charType::SPACECHAR;

		default:
			if (::IsCharAlphaNumericA(letter))
				return charType::ALPHANUMCHAR;
		break;
	}

	return charType::OTHERCHAR;
}


void getWords(HWND view, const UserSettings& settings, chunk_info& chunk)
{
	chunk.words.clear();

	for (int lineNum = 0; lineNum < chunk.lineCount; ++lineNum)
	{
		chunk.lineStartWordIdx[lineNum] = chunk.words.size();

		const int docLineNum = lineNum + chunk.lineStart;
		const int docLineStart = ::SendMessage(view, SCI_POSITIONFROMLINE, docLineNum, 0);
		const int docLineEnd = ::SendMessage(view, SCI_GETLINEENDPOSITION, docLineNum, 0);

		const std::vector<char> line = getText(view, docLineStart, docLineEnd);
		const int lineLen = static_cast<int>(line.size()) - 1;

		if (lineLen > 0)
		{
			Word word;
			word.type = getCharType(line[0]);
			word.hash = HASH(0, line[0]);
			word.line = lineNum;
			word.pos = 0;
			word.length = 1;

			for (int i = 1; i < lineLen; ++i)
			{
				const char l = line[i];
				charType newType = getCharType(l);

				if (newType == word.type)
				{
					++word.length;
					word.hash = HASH(word.hash, l);
				}
				else
				{
					if (!settings.IgnoreSpaces || word.type != charType::SPACECHAR)
						chunk.words.push_back(word);

					word.type = newType;
					word.hash = HASH(0, l);
					word.pos = i;
					word.length = 1;
				}
			}

			if (!settings.IgnoreSpaces || word.type != charType::SPACECHAR)
				chunk.words.push_back(word);
		}

		chunk.lineEndWordIdx[lineNum] = chunk.words.size();
	}
}


void compareLines(diff_edit& blockDiff1, diff_edit& blockDiff2, const chunk_info& chunk1, const chunk_info& chunk2)
{
	for (int line1 = 0; line1 < chunk1.lineCount; ++line1)
	{
		if (chunk1.lineMappings[line1] == -1)
			continue;

		const int line2 = chunk1.lineMappings[line1];

		const std::vector<Word> words1(&chunk1.words[chunk1.lineStartWordIdx[line1]],
				&chunk1.words[chunk1.lineEndWordIdx[line1]]);
		const std::vector<Word> words2(&chunk2.words[chunk2.lineStartWordIdx[line2]],
				&chunk2.words[chunk2.lineEndWordIdx[line2]]);

		diff_edit* pBlockDiff1 = &blockDiff1;
		diff_edit* pBlockDiff2 = &blockDiff2;

		const std::vector<Word>* pWords1 = &words1;
		const std::vector<Word>* pWords2 = &words2;

		const int* pLine1 = &line1;
		const int* pLine2 = &line2;

		if (words1.size() > words2.size())
		{
			std::swap(pBlockDiff1, pBlockDiff2);
			std::swap(pWords1, pWords2);
			std::swap(pLine1, pLine2);
		}

		// Compare the two lines
		const std::vector<diff_edit> lineDiff = DiffCalc<Word>(*pWords1, *pWords2)();

		const int lineDiffSize = static_cast<int>(lineDiff.size());

		if (lineDiffSize == 0)
			continue;

		bool line1Changed = false;
		bool line2Changed = false;

		for (int i = 0; i < lineDiffSize; ++i)
		{
			const diff_edit& ld = lineDiff[i];

			if (ld.type == diff_type::DIFF_DELETE)
			{
				for (int j = 0; j < ld.len; ++j)
				{
					const Word& word = (*pWords1)[ld.off + j];
					diff_change change;

					change.off = word.pos;
					change.len = word.length;
					change.line = *pLine1;
					change.matchedLine = *pLine2;

					pBlockDiff1->changes.emplace_back(change);
				}

				line1Changed = true;
			}
			else if (ld.type == diff_type::DIFF_INSERT)
			{
				for (int j = 0; j < ld.len; ++j)
				{
					const Word& word = (*pWords2)[ld.off + j];
					diff_change change;

					change.off = word.pos;
					change.len = word.length;
					change.line = *pLine2;
					change.matchedLine = *pLine1;

					pBlockDiff2->changes.emplace_back(change);
				}

				line2Changed = true;
			}
		}

		if (!line1Changed)
		{
			diff_change change;

			change.off = 0;
			change.len = 0;
			change.line = *pLine1;
			change.matchedLine = *pLine2;

			pBlockDiff1->changes.emplace_back(change);
		}

		if (!line2Changed)
		{
			diff_change change;

			change.off = 0;
			change.len = 0;
			change.line = *pLine2;
			change.matchedLine = *pLine1;

			pBlockDiff2->changes.emplace_back(change);
		}
	}
}


int markSection(HWND view, HWND otherView, int line, int length, diff_mark marker, bool addBlanks)
{
	if (length <= 0)
		return 0;

	for (int i = 0; i < length; ++i)
	{
		::SendMessage(view, SCI_MARKERADD, line + i, lineMark[marker]);
		::SendMessage(view, SCI_MARKERADD, line + i, symbolMark[marker]);
	}

	if (addBlanks)
	{
		addBlankSection(otherView, line, length);
		return length;
	}

	return 0;
}


int markSection(HWND view, int docOffset, const diff_edit& bd, diff_mark bdMark)
{
	const int blanksLen = bd.matchedLen - bd.len;

	int line = docOffset + bd.off;

	for (int i = 0; i < bd.len; ++i, ++line)
	{
		const diff_mark marker = bd.isMoved(i) ? MOVED_MARK : bdMark;

		::SendMessage(view, SCI_MARKERADD, line, lineMark[marker]);
		::SendMessage(view, SCI_MARKERADD, line, symbolMark[marker]);
	}

	if (blanksLen > 0)
	{
		addBlankSection(view, line, blanksLen);
		return blanksLen;
	}

	return 0;
}

}


// The returned bool is true if views are swapped and false otherwise
std::pair<std::vector<diff_edit>, bool>
	compareDocs(HWND& view1, HWND& view2, const UserSettings& settings, progress_ptr& progress)
{
	if (progress)
		progress->SetMaxCount(3);

	const std::vector<unsigned int> lineHashes1 = computeLineHashes(view1, settings);

	if (progress && !progress->Advance())
		return std::make_pair(std::vector<diff_edit>(), false);

	const std::vector<unsigned int> lineHashes2 = computeLineHashes(view2, settings);

	if (progress && !progress->Advance())
		return std::make_pair(std::vector<diff_edit>(), false);

	bool viewsSwapped = false;

	const std::vector<unsigned int>* pLineHashes1 = &lineHashes1;
	const std::vector<unsigned int>* pLineHashes2 = &lineHashes2;

	if (lineHashes1.size() > lineHashes2.size())
	{
		std::swap(view1, view2);
		std::swap(pLineHashes1, pLineHashes2);
		viewsSwapped = true;
	}

	return std::make_pair
			(DiffCalc<unsigned int>(*pLineHashes1, *pLineHashes2, settings.DetectMoves)(), viewsSwapped);
}


bool compareBlocks(HWND view1, HWND view2, const UserSettings& settings, diff_edit& blockDiff1, diff_edit& blockDiff2)
{
	diff_edit* pBlockDiff1 = &blockDiff1;
	diff_edit* pBlockDiff2 = &blockDiff2;

	if (blockDiff1.len > blockDiff2.len)
	{
		std::swap(view1, view2);
		std::swap(pBlockDiff1, pBlockDiff2);
	}

	chunk_info chunk1(pBlockDiff1->off, pBlockDiff1->len);
	chunk_info chunk2(pBlockDiff2->off, pBlockDiff2->len);

	getWords(view1, settings, chunk1);
	getWords(view2, settings, chunk2);

	// Compare the two chunks
	const std::vector<diff_edit> chunkDiff = DiffCalc<Word>(chunk1.words, chunk2.words)();

	const int chunkDiffSize = static_cast<int>(chunkDiff.size());

	if (chunkDiffSize == 0)
		return false;

	std::vector<std::vector<int>> linesConvergence(chunk1.lineCount, std::vector<int>(chunk2.lineCount, 0));

	// Use the MATCH results to synchronize line numbers (count the match length of each line)
	int wordOffset = 0;
	for (int i = 0; i < chunkDiffSize; ++i)
	{
		const diff_edit& cd = chunkDiff[i];

		if (cd.type == diff_type::DIFF_DELETE)
		{
			wordOffset -= cd.len;
		}
		else if (cd.type == diff_type::DIFF_INSERT)
		{
			wordOffset += cd.len;
		}
		else // diff_type::DIFF_MATCH
		{
			for (int wordIdx = cd.off; wordIdx < (cd.off + cd.len); ++wordIdx)
			{
				const Word& word1 = chunk1.words[wordIdx];
				const Word& word2 = chunk2.words[wordIdx + wordOffset];

				if (word1.type != charType::SPACECHAR)
					linesConvergence[word1.line][word2.line] += word1.length;
			}
		}
	}

	// Select the line with the most matches (as length)
	for (int line1 = 0; line1 < chunk1.lineCount; ++line1)
	{
		if (pBlockDiff1->isMoved(line1))
			continue;

		int maxConvergence = 0;
		int line2 = 0;

		for (int i = 0; i < chunk2.lineCount; ++i)
		{
			if (!pBlockDiff2->isMoved(i) && linesConvergence[line1][i] > maxConvergence)
			{
				line2 = i;
				maxConvergence = linesConvergence[line1][i];
			}
		}

		// Make sure that the line is matched and the other line is not already matched
		if (maxConvergence == 0 || chunk2.lineMappings[line2] != -1)
			continue;

		int line1Size = 0;
		for (int i = chunk1.lineStartWordIdx[line1]; i < chunk1.lineEndWordIdx[line1]; ++i)
		{
			const Word& word1 = chunk1.words[i];

			if (word1.type != charType::SPACECHAR)
				line1Size += word1.length;
		}

		// Is enough portion of the line matched to be significant?
		if (line1Size && maxConvergence > (line1Size / 3))
		{
			chunk1.lineMappings[line1] = line2;
			chunk2.lineMappings[line2] = line1;
		}
	}

	compareLines(*pBlockDiff1, *pBlockDiff2, chunk1, chunk2);

	return true;
}


// Mark all line differences
bool showDiffs(HWND view1, HWND view2, const std::pair<std::vector<diff_edit>, bool>& cmpResults,
		progress_ptr& progress)
{
	const std::vector<diff_edit>& blockDiff = cmpResults.first;
	const bool viewsSwapped = cmpResults.second;

	const diff_mark deletedMark	= viewsSwapped ? ADDED_MARK : DELETED_MARK;
	const diff_mark addedMark	= viewsSwapped ? DELETED_MARK : ADDED_MARK;

	const int blockDiffSize = static_cast<int>(blockDiff.size());

	if (progress)
		progress->SetMaxCount(blockDiffSize);

	int doc1Offset = 0;
	int doc2Offset = 0;

	for (int i = 0; i < blockDiffSize; ++i)
	{
		const diff_edit& bd = blockDiff[i];

		if (bd.type != diff_type::DIFF_MATCH)
		{
			int* docOffset;
			int* otherDocOffset;

			HWND view;
			HWND otherView;

			diff_mark bdMark;

			int matchedBdLine;

			if (bd.type == diff_type::DIFF_DELETE)
			{
				docOffset		= &doc1Offset;
				otherDocOffset	= &doc2Offset;
				view			= view1;
				otherView		= view2;
				bdMark			= deletedMark;
				matchedBdLine	= -1;
			}
			else // diff_type::DIFF_INSERT
			{
				docOffset		= &doc2Offset;
				otherDocOffset	= &doc1Offset;
				view			= view2;
				otherView		= view1;
				bdMark			= addedMark;
				matchedBdLine	= bd.matchedOff + *otherDocOffset;
			}

			const int bdLine = bd.off + *docOffset;

			if (bd.changes.empty())
			{
				*docOffset += markSection(view, *docOffset, bd, bdMark);

				if (!bd.matchedLen)
				{
					addBlankSection(otherView, bdLine, bd.len);
					*otherDocOffset += bd.len;
				}
			}
			else
			{
				int sectionLine = 0;
				int sectionLen = 0;

				diff_mark sectionMark = END_MARK;

				bool addBlanks = false;

				int line = bdLine;
				int addedLines = 0;

				for (int j = 0; j < bd.len; ++j, ++sectionLen, ++line)
				{
					if (bd.isMoved(j))
					{
						if (sectionMark != MOVED_MARK)
						{
							addedLines +=
									markSection(view, otherView, sectionLine, sectionLen, sectionMark, addBlanks);
							sectionLine	= line;
							sectionLen	= 0;
							sectionMark	= MOVED_MARK;
							addBlanks	= true;
						}

						continue;
					}

					const int changeCount = static_cast<int>(bd.changes.size());
					bool lineChanged = false;
					int linePos = 0;

					for (int k = 0; k < changeCount; ++k)
					{
						const diff_change& change = bd.changes[k];

						if (change.line == j)
						{
							if (!lineChanged)
							{
								if (matchedBdLine != -1 && change.line <= change.matchedLine)
									line = matchedBdLine + change.matchedLine + addedLines;
								linePos = ::SendMessage(view, SCI_POSITIONFROMLINE, line, 0);
								lineChanged = true;
							}

							if (change.len)
								markTextAsChanged(view, linePos + change.off, change.len);
						}
					}

					if (lineChanged)
					{
						if (sectionMark != CHANGED_MARK || line != sectionLine + sectionLen)
						{
							addedLines +=
									markSection(view, otherView, sectionLine, sectionLen, sectionMark, addBlanks);
							sectionLine	= line;
							sectionLen	= 0;
							sectionMark	= CHANGED_MARK;
							addBlanks	= false;
						}

						line = bdLine + j;
					}
					else
					{
						if (sectionMark != bdMark)
						{
							addedLines +=
									markSection(view, otherView, sectionLine, sectionLen, sectionMark, addBlanks);
							sectionLine	= line;
							sectionLen	= 0;
							sectionMark	= bdMark;
							addBlanks	= true;
						}
					}
				}

				addedLines += markSection(view, otherView, sectionLine, sectionLen, sectionMark, addBlanks);
				*otherDocOffset += addedLines;
			}
		}

		if (progress && !progress->Advance())
			return false;
	}

	if (progress && !progress->NextPhase())
		return false;

	return true;
}
