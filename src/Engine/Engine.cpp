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


std::vector<unsigned int> computeLineHashes(DocCmpInfo& doc, const UserSettings& settings)
{
	int docLines = ::SendMessage(doc.view, SCI_GETLENGTH, 0, 0);

	if (docLines)
		docLines = ::SendMessage(doc.view, SCI_GETLINECOUNT, 0, 0);

	if (doc.section.len <= 0 || doc.section.len > docLines)
		doc.section.len = docLines;

	std::vector<unsigned int> lineHashes(doc.section.len);

	for (int lineNum = 0; lineNum < doc.section.len; ++lineNum)
	{
		const int lineStart = ::SendMessage(doc.view, SCI_POSITIONFROMLINE, lineNum + doc.section.off, 0);
		const int lineEnd = ::SendMessage(doc.view, SCI_GETLINEENDPOSITION, lineNum + doc.section.off, 0);

		unsigned int hash = 0;

		if (lineEnd - lineStart)
		{
			const std::vector<char> line = getText(doc.view, lineStart, lineEnd);
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


void compareLines(diff_info& blockDiff1, diff_info& blockDiff2, const chunk_info& chunk1, const chunk_info& chunk2)
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

		diff_info* pBlockDiff1 = &blockDiff1;
		diff_info* pBlockDiff2 = &blockDiff2;

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
		const std::vector<diff_info> lineDiff = DiffCalc<Word>(*pWords1, *pWords2)();

		const int lineDiffSize = static_cast<int>(lineDiff.size());

		if (lineDiffSize == 0)
			continue;

		pBlockDiff1->changedLines.emplace_back(*pLine1);
		pBlockDiff2->changedLines.emplace_back(*pLine2);

		for (int i = 0; i < lineDiffSize; ++i)
		{
			const diff_info& ld = lineDiff[i];

			if (ld.type == diff_type::DIFF_DELETE)
			{
				section_t change;

				change.off = (*pWords1)[ld.off].pos;
				change.len = (*pWords1)[ld.off + ld.len - 1].pos - change.off + (*pWords1)[ld.off + ld.len - 1].length;

				pBlockDiff1->changedLines.back().changes.emplace_back(change);
			}
			else if (ld.type == diff_type::DIFF_INSERT)
			{
				section_t change;

				change.off = (*pWords2)[ld.off].pos;
				change.len = (*pWords2)[ld.off + ld.len - 1].pos - change.off + (*pWords2)[ld.off + ld.len - 1].length;

				pBlockDiff2->changedLines.back().changes.emplace_back(change);
			}
		}
	}
}


void markSection(std::pair<HWND, HWND>& views, const diff_info& bd, const std::pair<diff_mark, diff_mark>& bdMarks,
		const std::pair<section_t, section_t>& sections, std::pair<int*, int*>& addedBlanks)
{
	const int startLine = bd.off + *addedBlanks.first + sections.first.off;

	int line = startLine;
	int lenDiff = sections.second.len - sections.first.len;
	int endOff = sections.first.off + sections.first.len;

	for (int i = sections.first.off; i < endOff; ++i, ++line)
	{
		const diff_mark marker = bd.isMoved(i) ? MOVED_MARK : bdMarks.first;

		::SendMessage(views.first, SCI_MARKERADD, line, lineMark[marker]);
		::SendMessage(views.first, SCI_MARKERADD, line, symbolMark[marker]);
	}

	if (lenDiff > 0)
	{
		addBlankSection(views.first, line, lenDiff);
		*addedBlanks.first += lenDiff;
	}

	line = startLine;
	lenDiff = -lenDiff;
	endOff = sections.second.off + sections.second.len;

	for (int i = sections.second.off; i < endOff; ++i, ++line)
	{
		const diff_mark marker = bd.matchedDiff->isMoved(i) ? MOVED_MARK : bdMarks.second;

		::SendMessage(views.second, SCI_MARKERADD, line, lineMark[marker]);
		::SendMessage(views.second, SCI_MARKERADD, line, symbolMark[marker]);
	}

	if (lenDiff > 0)
	{
		addBlankSection(views.second, line, lenDiff);
		*addedBlanks.second += lenDiff;
	}
}


void markLineDiffs(std::pair<HWND, HWND>& views,
		const std::pair<const diff_line&, const diff_line&> changedLines, const int line)
{
	int linePos = ::SendMessage(views.first, SCI_POSITIONFROMLINE, line, 0);

	for (const auto& change : changedLines.first.changes)
		markTextAsChanged(views.first, linePos + change.off, change.len);

	::SendMessage(views.first, SCI_MARKERADD, line, lineMark[CHANGED_MARK]);
	::SendMessage(views.first, SCI_MARKERADD, line, symbolMark[CHANGED_MARK]);

	linePos = ::SendMessage(views.second, SCI_POSITIONFROMLINE, line, 0);

	for (const auto& change : changedLines.second.changes)
		markTextAsChanged(views.second, linePos + change.off, change.len);

	::SendMessage(views.second, SCI_MARKERADD, line, lineMark[CHANGED_MARK]);
	::SendMessage(views.second, SCI_MARKERADD, line, symbolMark[CHANGED_MARK]);
}

}


// The returned bool is true if views are swapped and false otherwise
std::pair<std::vector<diff_info>, bool>
	compareDocs(DocCmpInfo& doc1, DocCmpInfo& doc2, const UserSettings& settings, progress_ptr& progress)
{
	if (progress)
		progress->SetMaxCount(3);

	const std::vector<unsigned int> lineHashes1 = computeLineHashes(doc1, settings);

	if (progress && !progress->Advance())
		return std::make_pair(std::vector<diff_info>(), false);

	const std::vector<unsigned int> lineHashes2 = computeLineHashes(doc2, settings);

	if (progress && !progress->Advance())
		return std::make_pair(std::vector<diff_info>(), false);

	bool viewsSwapped = false;

	const std::vector<unsigned int>* pLineHashes1 = &lineHashes1;
	const std::vector<unsigned int>* pLineHashes2 = &lineHashes2;

	if (lineHashes1.size() > lineHashes2.size())
	{
		std::swap(doc1, doc2);
		std::swap(pLineHashes1, pLineHashes2);
		viewsSwapped = true;
	}

	std::pair<std::vector<diff_info>, bool> cmpResults =
			std::make_pair(DiffCalc<unsigned int>(*pLineHashes1, *pLineHashes2, settings.DetectMoves)(), viewsSwapped);

	std::vector<diff_info>& blockDiff = cmpResults.first;

	const int startOff = (doc1.section.off > doc2.section.off) ? doc1.section.off : doc2.section.off;

	if (startOff)
	{
		for (auto& bd : blockDiff)
			bd.off += startOff;
	}

	// Align views if comparison start offsets differ
	if (doc1.section.off != doc2.section.off)
	{
		if (doc1.section.off > doc2.section.off)
			addBlankSection(doc2.view, doc2.section.off, doc1.section.off - doc2.section.off);
		else
			addBlankSection(doc1.view, doc1.section.off, doc2.section.off - doc1.section.off);
	}

	return std::move(cmpResults);
}


bool compareBlocks(HWND view1, HWND view2, const UserSettings& settings, diff_info& blockDiff1, diff_info& blockDiff2)
{
	diff_info* pBlockDiff1 = &blockDiff1;
	diff_info* pBlockDiff2 = &blockDiff2;

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
	const std::vector<diff_info> chunkDiff = DiffCalc<Word>(chunk1.words, chunk2.words)();

	const int chunkDiffSize = static_cast<int>(chunkDiff.size());

	if (chunkDiffSize == 0)
		return false;

	std::vector<std::vector<int>> linesConvergence(chunk1.lineCount, std::vector<int>(chunk2.lineCount, 0));

	// Use the MATCH results to synchronize line numbers (count the match length of each line)
	int wordOffset = 0;
	for (int i = 0; i < chunkDiffSize; ++i)
	{
		const diff_info& cd = chunkDiff[i];

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
bool showDiffs(const DocCmpInfo& doc1, const DocCmpInfo& doc2,
		const std::pair<std::vector<diff_info>, bool>& cmpResults, progress_ptr& progress)
{
	const std::vector<diff_info>& blockDiff = cmpResults.first;

	const diff_mark deletedMark	= cmpResults.second ? ADDED_MARK : DELETED_MARK;
	const diff_mark addedMark	= cmpResults.second ? DELETED_MARK : ADDED_MARK;

	const int blockDiffSize = static_cast<int>(blockDiff.size());

	if (progress)
		progress->SetMaxCount(blockDiffSize);

	int addedBlanks1 = 0;
	int addedBlanks2 = 0;

	for (int i = 0; i < blockDiffSize; ++i)
	{
		const diff_info& bd = blockDiff[i];

		if (bd.type != diff_type::DIFF_MATCH)
		{
			std::pair<HWND, HWND> views;
			std::pair<int*, int*> addedBlanks;
			std::pair<diff_mark, diff_mark> bdMarks;

			if (bd.type == diff_type::DIFF_DELETE)
			{
				addedBlanks.first	= &addedBlanks1;
				addedBlanks.second	= &addedBlanks2;
				views.first			= doc1.view;
				views.second		= doc2.view;
				bdMarks.first		= deletedMark;
				bdMarks.second		= addedMark;
			}
			else // diff_type::DIFF_INSERT
			{
				addedBlanks.first	= &addedBlanks2;
				addedBlanks.second	= &addedBlanks1;
				views.first			= doc2.view;
				views.second		= doc1.view;
				bdMarks.first		= addedMark;
				bdMarks.second		= deletedMark;
			}

			std::pair<section_t, section_t> sections( { 0, 0 }, { 0, 0 } );

			const int changedLinesCount = static_cast<int>(bd.changedLines.size());

			for (int j = 0; j < changedLinesCount; ++j)
			{
				const std::pair<const diff_line&, const diff_line&>
						changedLines(bd.changedLines[j], bd.matchedDiff->changedLines[j]);

				if (sections.first.off != changedLines.first.line ||
					sections.second.off != changedLines.second.line)
				{
					sections.first.len = changedLines.first.line - sections.first.off;
					sections.second.len = changedLines.second.line - sections.second.off;

					markSection(views, bd, bdMarks, sections, addedBlanks);
				}

				markLineDiffs(views, changedLines, bd.off + *addedBlanks.first + changedLines.first.line);

				sections.first.off = changedLines.first.line + 1;
				sections.second.off = changedLines.second.line + 1;
			}

			sections.first.len = bd.len - sections.first.off;

			if (bd.matchedDiff)
			{
				sections.second.len = bd.matchedDiff->len - sections.second.off;
				++i;
			}
			else
			{
				sections.second.len = 0;
			}

			markSection(views, bd, bdMarks, sections, addedBlanks);
		}

		if (progress && !progress->Advance())
			return false;
	}

	if (progress && !progress->NextPhase())
		return false;

	return true;
}
