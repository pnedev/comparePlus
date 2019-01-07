/*
 * This file is part of Compare plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2018 Pavel Nedev (pg.nedev@gmail.com)
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

#include <climits>
#include <exception>
#include <cstdint>
#include <utility>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <map>
#include <algorithm>

#include <windows.h>

#include "Engine.h"
#include "diff.h"
#include "ProgressDlg.h"


namespace {

enum class charType
{
	SPACECHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


struct DocCmpInfo
{
	int			view;
	section_t	section;

	int			blockDiffMask;

	std::unordered_set<int>	nonUniqueLines;
};


struct diffLine
{
	diffLine(int lineNum) : line(lineNum) {}

	int line;
	std::vector<section_t> changes;
};


struct blockDiffInfo
{
	const diff_info<blockDiffInfo>*	matchBlock {nullptr};

	std::vector<diffLine>	changedLines;
	std::vector<section_t>	moves;

	inline int movedSection(int line) const
	{
		for (const auto& move: moves)
		{
			if (line >= move.off && line < move.off + move.len)
				return move.len;
		}

		return 0;
	}

	inline bool getNextUnmoved(int& line) const
	{
		for (const auto& move: moves)
		{
			if (line >= move.off && line < move.off + move.len)
			{
				line = move.off + move.len;
				return true;
			}
		}

		return false;
	}
};


using diffInfo = diff_info<blockDiffInfo>;


struct CompareInfo
{
	// Input data
	DocCmpInfo				doc1;
	DocCmpInfo				doc2;

	bool					selectionCompare;

	// Output data - filled by the compare engine
	std::vector<diffInfo>	blockDiffs;
};


struct MatchInfo
{
	int			lookupOff;
	diffInfo*	matchDiff;
	int			matchOff;
	int			matchLen;
};


struct Word
{
	charType type;

	int line;
	int pos;
	int length;

	uint64_t hash;

	inline bool operator==(const Word& rhs) const
	{
		return (hash == rhs.hash);
	}

	inline bool operator!=(const Word& rhs) const
	{
		return (hash != rhs.hash);
	}

	inline bool operator==(uint64_t rhs) const
	{
		return (hash == rhs);
	}

	inline bool operator!=(uint64_t rhs) const
	{
		return (hash != rhs);
	}
};


const uint64_t cHashSeed = 0x84222325;

inline uint64_t Hash(uint64_t hval, char letter)
{
	hval ^= static_cast<uint64_t>(letter);

	hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);

	return hval;
}


std::vector<uint64_t> computeLineHashes(DocCmpInfo& doc, const CompareOptions& options)
{
	const int monitorCancelEveryXLine = 500;

	progress_ptr& progress = ProgressDlg::Get();

	int lineCount = CallScintilla(doc.view, SCI_GETLENGTH, 0, 0);

	if (lineCount)
		lineCount = CallScintilla(doc.view, SCI_GETLINECOUNT, 0, 0);
	else
		return std::vector<uint64_t>{};

	if ((doc.section.len <= 0) || (doc.section.off + doc.section.len > lineCount))
		doc.section.len = lineCount - doc.section.off;

	if (progress)
		progress->SetMaxCount((doc.section.len / monitorCancelEveryXLine) + 1);

	std::vector<uint64_t> lineHashes(doc.section.len, cHashSeed);

	for (int lineNum = 0; lineNum < doc.section.len; ++lineNum)
	{
		if (progress && (lineNum % monitorCancelEveryXLine == 0) && !progress->Advance())
			return std::vector<uint64_t>{};

		const int lineStart = CallScintilla(doc.view, SCI_POSITIONFROMLINE, lineNum + doc.section.off, 0);
		const int lineEnd = CallScintilla(doc.view, SCI_GETLINEENDPOSITION, lineNum + doc.section.off, 0);

		if (lineEnd - lineStart)
		{
			std::vector<char> line = getText(doc.view, lineStart, lineEnd);
			const int lineLen = static_cast<int>(line.size()) - 1;

			if (options.ignoreCase)
				toLowerCase(line);

			for (int i = 0; i < lineLen; ++i)
			{
				if (options.ignoreSpaces && (line[i] == ' ' || line[i] == '\t'))
					continue;

				lineHashes[lineNum] = Hash(lineHashes[lineNum], line[i]);
			}
		}
	}

	if (doc.section.len && lineHashes.back() == cHashSeed)
	{
		lineHashes.pop_back();
		--doc.section.len;
	}

	return lineHashes;
}


charType getCharType(char letter)
{
	if (letter == ' ' || letter == '\t')
		return charType::SPACECHAR;

	if (::IsCharAlphaNumericA(letter) || letter == '_')
		return charType::ALPHANUMCHAR;

	return charType::OTHERCHAR;
}


std::vector<std::vector<Word>> getWords(int lineOffset, int lineCount, int view, const CompareOptions& options)
{
	std::vector<std::vector<Word>> words(lineCount);

	for (int lineNum = 0; lineNum < lineCount; ++lineNum)
	{
		const int docLineNum = lineNum + lineOffset;
		const int docLineStart = CallScintilla(view, SCI_POSITIONFROMLINE, docLineNum, 0);
		const int docLineEnd = CallScintilla(view, SCI_GETLINEENDPOSITION, docLineNum, 0);

		if (docLineEnd - docLineStart)
		{
			std::vector<char> line = getText(view, docLineStart, docLineEnd);
			const int lineLen = static_cast<int>(line.size()) - 1;

			if (options.ignoreCase)
				toLowerCase(line);

			Word word;
			word.type = getCharType(line[0]);
			word.hash = Hash(cHashSeed, line[0]);
			word.line = lineNum;
			word.pos = 0;
			word.length = 1;

			for (int i = 1; i < lineLen; ++i)
			{
				charType newType = getCharType(line[i]);

				if (newType == word.type)
				{
					++word.length;
					word.hash = Hash(word.hash, line[i]);
				}
				else
				{
					if (!options.ignoreSpaces || word.type != charType::SPACECHAR)
						words[lineNum].push_back(word);

					word.type = newType;
					word.hash = Hash(cHashSeed, line[i]);
					word.pos = i;
					word.length = 1;
				}
			}

			if (!options.ignoreSpaces || word.type != charType::SPACECHAR)
				words[lineNum].push_back(word);
		}
	}

	return words;
}


// Scan for the best single matching block in the other file
void findBestMatch(const CompareInfo& cmpInfo,
		const std::vector<uint64_t>& lineHashes1, const std::vector<uint64_t>& lineHashes2,
		const diffInfo& lookupDiff, int lookupOff, MatchInfo& mi)
{
	mi.matchLen		= 0;
	mi.matchDiff	= nullptr;

	const std::vector<uint64_t>* pLookupLines;
	const std::vector<uint64_t>* pMatchLines;
	diff_type matchType;

	if (lookupDiff.type == diff_type::DIFF_IN_1)
	{
		pLookupLines	= &lineHashes1;
		pMatchLines		= &lineHashes2;
		matchType		= diff_type::DIFF_IN_2;
	}
	else
	{
		pLookupLines	= &lineHashes2;
		pMatchLines		= &lineHashes1;
		matchType		= diff_type::DIFF_IN_1;
	}

	int minMatchLen = 1;

	for (const diffInfo& matchDiff: cmpInfo.blockDiffs)
	{
		if (matchDiff.type != matchType || matchDiff.len < minMatchLen)
			continue;

		int matchLastUnmoved = 0;

		for (int matchOff = 0; matchOff < matchDiff.len; ++matchOff)
		{
			if ((*pLookupLines)[lookupDiff.off + lookupOff] != (*pMatchLines)[matchDiff.off + matchOff])
				continue;

			if (matchDiff.info.getNextUnmoved(matchOff))
			{
				matchLastUnmoved = matchOff;
				--matchOff;
				continue;
			}

			int lookupStart	= lookupOff - 1;
			int matchStart	= matchOff - 1;

			// Check for the beginning of the matched block (containing lookupOff element)
			for (; lookupStart >= 0 && matchStart >= matchLastUnmoved &&
					(*pLookupLines)[lookupDiff.off + lookupStart] == (*pMatchLines)[matchDiff.off + matchStart] &&
					!lookupDiff.info.movedSection(lookupStart);
					--lookupStart, --matchStart);

			++lookupStart;
			++matchStart;

			int lookupEnd	= lookupOff + 1;
			int matchEnd	= matchOff + 1;

			// Check for the end of the matched block (containing lookupOff element)
			for (; lookupEnd < lookupDiff.len && matchEnd < matchDiff.len &&
					(*pLookupLines)[lookupDiff.off + lookupEnd] == (*pMatchLines)[matchDiff.off + matchEnd] &&
					!lookupDiff.info.movedSection(lookupEnd) && !matchDiff.info.movedSection(matchEnd);
					++lookupEnd, ++matchEnd);

			const int matchLen = lookupEnd - lookupStart;

			if (mi.matchLen < matchLen)
			{
				mi.lookupOff	= lookupStart;
				mi.matchDiff	= const_cast<diffInfo*>(&matchDiff);
				mi.matchOff		= matchStart;
				mi.matchLen		= matchLen;

				minMatchLen		= matchLen;
			}
			else if (mi.matchLen == matchLen)
			{
				mi.matchDiff = nullptr;
			}
		}
	}
}


// Recursively resolve the best match
bool resolveMatch(const CompareInfo& cmpInfo,
		const std::vector<uint64_t>& lineHashes1, const std::vector<uint64_t>& lineHashes2,
		diffInfo& lookupDiff, int lookupOff, MatchInfo& lookupMi)
{
	bool ret = false;

	if (lookupMi.matchDiff)
	{
		lookupOff = lookupMi.matchOff + (lookupOff - lookupMi.lookupOff);

		MatchInfo reverseMi;
		findBestMatch(cmpInfo, lineHashes1, lineHashes2, *(lookupMi.matchDiff), lookupOff, reverseMi);

		if (reverseMi.matchDiff == &lookupDiff)
		{
			lookupDiff.info.moves.emplace_back(lookupMi.lookupOff, lookupMi.matchLen);
			lookupMi.matchDiff->info.moves.emplace_back(lookupMi.matchOff, lookupMi.matchLen);
			ret = true;
		}
		else if (reverseMi.matchDiff)
		{
			ret = resolveMatch(cmpInfo, lineHashes1, lineHashes2, *(lookupMi.matchDiff), lookupOff, reverseMi);
			lookupMi.matchLen = 0;
		}
	}

	return ret;
}


void findMoves(CompareInfo& cmpInfo,
		const std::vector<uint64_t>& lineHashes1, const std::vector<uint64_t>& lineHashes2)
{
	// LOGD("FIND MOVES\n");

	bool repeat = true;

	while (repeat)
	{
		repeat = false;

		for (diffInfo& lookupDiff: cmpInfo.blockDiffs)
		{
			if (lookupDiff.type != diff_type::DIFF_IN_1)
				continue;

			// LOGD("DIFF_IN_1 offset: " + std::to_string(lookupDiff.off + 1) + "\n");

			// Go through all lookupDiff's elements and check if each is matched
			for (int lookupEi = 0; lookupEi < lookupDiff.len; ++lookupEi)
			{
				// Skip already detected moves
				if (lookupDiff.info.getNextUnmoved(lookupEi))
				{
					--lookupEi;
					continue;
				}

				// LOGD("line offset: " + std::to_string(lookupEi) + "\n");

				MatchInfo mi;
				findBestMatch(cmpInfo, lineHashes1, lineHashes2, lookupDiff, lookupEi, mi);

				if (resolveMatch(cmpInfo, lineHashes1, lineHashes2, lookupDiff, lookupEi, mi))
				{
					repeat = true;

					if (mi.matchLen)
						lookupEi = mi.lookupOff + mi.matchLen - 1;
					else
						--lookupEi;

					// LOGD("move match found, next line offset: " + std::to_string(lookupEi + 1) + "\n");
				}
			}
		}
	}
}


void findUniqueLines(CompareInfo& cmpInfo,
		const std::vector<uint64_t>& lineHashes1, const std::vector<uint64_t>& lineHashes2)
{
	std::unordered_map<uint64_t, std::vector<int>> doc1LinesMap;

	int sectionEnd = cmpInfo.doc1.section.off + cmpInfo.doc1.section.len;
	if (sectionEnd > static_cast<int>(lineHashes1.size()))
		sectionEnd = static_cast<int>(lineHashes1.size());

	for (int i = cmpInfo.doc1.section.off; i < sectionEnd; ++i)
	{
		auto insertPair = doc1LinesMap.emplace(lineHashes1[i], std::vector<int>{i});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(i);
	}

	sectionEnd = cmpInfo.doc2.section.off + cmpInfo.doc2.section.len;
	if (sectionEnd > static_cast<int>(lineHashes2.size()))
		sectionEnd = static_cast<int>(lineHashes2.size());

	for (int i = cmpInfo.doc2.section.off; i < sectionEnd; ++i)
	{
		std::unordered_map<uint64_t, std::vector<int>>::iterator doc1it = doc1LinesMap.find(lineHashes2[i]);

		if (doc1it != doc1LinesMap.end())
		{
			cmpInfo.doc2.nonUniqueLines.emplace(i);

			auto insertPair = cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[0]);
			if (insertPair.second)
			{
				for (unsigned int j = 1; j < doc1it->second.size(); ++j)
					cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[j]);
			}
		}
	}
}


void compareLines(const DocCmpInfo& doc1, const DocCmpInfo& doc2, const CompareOptions& options,
		diffInfo& blockDiff1, diffInfo& blockDiff2,
		const std::vector<std::vector<Word>>& chunk1, const std::vector<std::vector<Word>>& chunk2,
		const std::map<int, std::pair<int, int>>& lineMappings)
{
	int lastLine2 = -1;

	for (const auto& lm: lineMappings)
	{
		const DocCmpInfo* pDoc1 = &doc1;
		const DocCmpInfo* pDoc2 = &doc2;

		diffInfo* pBlockDiff1 = &blockDiff1;
		diffInfo* pBlockDiff2 = &blockDiff2;

		// lines1 are stored in ascending order and to have a match lines2 must also be in ascending order
		if (lm.second.second <= lastLine2)
			continue;

		int line1 = lm.first;
		int line2 = lm.second.second;

		lastLine2 = line2;

		const std::vector<Word>* pLine1 = &chunk1[line1];
		const std::vector<Word>* pLine2 = &chunk2[line2];

		if (pLine1->size() < pLine2->size())
		{
			std::swap(pDoc1, pDoc2);
			std::swap(pBlockDiff1, pBlockDiff2);
			std::swap(pLine1, pLine2);
			std::swap(line1, line2);
		}

		const std::vector<diff_info<void>> linesDiff = DiffCalc<Word>(*pLine1, *pLine2)();
		const int linesDiffSize = static_cast<int>(linesDiff.size());

		if (linesDiffSize == 1 && linesDiff[0].type == diff_type::DIFF_MATCH)
			continue;

		pBlockDiff1->info.changedLines.emplace_back(line1);
		pBlockDiff2->info.changedLines.emplace_back(line2);

		const int lineOff1 = CallScintilla(pDoc1->view, SCI_POSITIONFROMLINE, line1 + pBlockDiff1->off, 0);
		const int lineOff2 = CallScintilla(pDoc2->view, SCI_POSITIONFROMLINE, line2 + pBlockDiff2->off, 0);

		for (int i = 0; i < linesDiffSize; ++i)
		{
			const auto& ld = linesDiff[i];

			if (ld.type == diff_type::DIFF_IN_1)
			{
				// Check if the DIFF_IN_1 / DIFF_IN_2 pair includes changed words or it's a completely new section
				if (i + 1 < linesDiffSize && linesDiff[i + 1].type == diff_type::DIFF_IN_2)
				{
					const auto& ld2 = linesDiff[i + 1];

					int off1 = (*pLine1)[ld.off].pos;
					int end1 = (*pLine1)[ld.off + ld.len - 1].pos + (*pLine1)[ld.off + ld.len - 1].length;

					int off2 = (*pLine2)[ld2.off].pos;
					int end2 = (*pLine2)[ld2.off + ld2.len - 1].pos + (*pLine2)[ld2.off + ld2.len - 1].length;

					std::vector<char> sec1 = getText(pDoc1->view, off1 + lineOff1, end1 + lineOff1);
					std::vector<char> sec2 = getText(pDoc2->view, off2 + lineOff2, end2 + lineOff2);

					if (options.ignoreCase)
					{
						toLowerCase(sec1);
						toLowerCase(sec2);
					}

					const auto* pSec1 = &sec1;
					const auto* pSec2 = &sec2;

					diffInfo* pBD1 = pBlockDiff1;
					diffInfo* pBD2 = pBlockDiff2;

					if (pSec1->size() < pSec2->size())
					{
						std::swap(pSec1, pSec2);
						std::swap(pBD1, pBD2);
						std::swap(off1, off2);
						std::swap(end1, end2);
					}

					// Compare changed words sections
					const std::vector<diff_info<void>> wordsDiff = DiffCalc<char>(*pSec1, *pSec2)(false);

					const int maxLen = (end1 - off1 > end2 - off2) ? end1 - off1 : end2 - off2;

					int matchLen = 0;
					int matchSections = 0;

					for (const auto& wd: wordsDiff)
					{
						if (wd.type == diff_type::DIFF_MATCH)
						{
							matchLen += wd.len;
							++matchSections;
						}
					}

					if ((matchLen / matchSections) && ((((matchLen / matchSections) * 100) / maxLen) >= 30))
					{
						for (const auto& wd: wordsDiff)
						{
							if (wd.type == diff_type::DIFF_IN_1)
							{
								section_t change;

								change.off = wd.off + off1;
								change.len = wd.len;

								pBD1->info.changedLines.back().changes.emplace_back(change);
							}
							else if (wd.type == diff_type::DIFF_IN_2)
							{
								section_t change;

								change.off = wd.off + off2;
								change.len = wd.len;

								pBD2->info.changedLines.back().changes.emplace_back(change);
							}
						}

						++i;
						continue;
					}
				}

				section_t change;

				change.off = (*pLine1)[ld.off].pos;
				change.len = (*pLine1)[ld.off + ld.len - 1].pos - change.off + (*pLine1)[ld.off + ld.len - 1].length;

				pBlockDiff1->info.changedLines.back().changes.emplace_back(change);
			}
			else if (ld.type == diff_type::DIFF_IN_2)
			{
				section_t change;

				change.off = (*pLine2)[ld.off].pos;
				change.len = (*pLine2)[ld.off + ld.len - 1].pos - change.off + (*pLine2)[ld.off + ld.len - 1].length;

				pBlockDiff2->info.changedLines.back().changes.emplace_back(change);
			}
		}
	}
}


void compareBlocks(const DocCmpInfo& doc1, const DocCmpInfo& doc2, const CompareOptions& options,
		diffInfo& blockDiff1, diffInfo& blockDiff2)
{
	const std::vector<std::vector<Word>> chunk1 = getWords(blockDiff1.off, blockDiff1.len, doc1.view, options);
	const std::vector<std::vector<Word>> chunk2 = getWords(blockDiff2.off, blockDiff2.len, doc2.view, options);

	const int linesCount1 = static_cast<int>(chunk1.size());
	const int linesCount2 = static_cast<int>(chunk2.size());

	struct conv_key
	{
		int convergence;
		int line1;
		int line2;

		conv_key(int c, int l1, int l2) : convergence(c), line1(l1), line2(l2)
		{}

		bool operator<(const conv_key& rhs) const
		{
			return ((convergence > rhs.convergence) ||
					((convergence == rhs.convergence) && ((line1 < rhs.line1) ||
						((line1 == rhs.line1) && ((line2 < rhs.line2))))));
		}
	};

	std::set<conv_key> orderedLinesConvergence;

	for (int line1 = 0; line1 < linesCount1; ++line1)
	{
		if (chunk1[line1].empty())
			continue;

		if (blockDiff1.info.getNextUnmoved(line1))
		{
			--line1;
			continue;
		}

		int line1Len = 0;
		for (const auto& word: chunk1[line1])
			line1Len += word.length;

		for (int line2 = 0; line2 < linesCount2; ++line2)
		{
			if (chunk2[line2].empty())
				continue;

			if (blockDiff2.info.getNextUnmoved(line2))
			{
				--line2;
				continue;
			}

			const std::vector<Word>* pLine1 = &chunk1[line1];
			const std::vector<Word>* pLine2 = &chunk2[line2];

			if (pLine1->size() < pLine2->size())
				std::swap(pLine1, pLine2);

			if (pLine1->size() > 2 * pLine2->size())
				continue;

			const std::vector<diff_info<void>> linesDiff = DiffCalc<Word>(*pLine1, *pLine2)();

			int line2Len = 0;
			for (const auto& word: chunk2[line2])
				line2Len += word.length;

			if (line1Len < line2Len)
				line1Len = line2Len;

			int linesConvergence = 0;		// Convergence by words count
			int linesLenConvergence = 0;	// Convergence by length (symbols/chars count)

			for (const auto& ld: linesDiff)
			{
				if (ld.type == diff_type::DIFF_MATCH)
				{
					linesConvergence += ld.len;

					for (int i = 0; i < ld.len; ++i)
						linesLenConvergence += (*pLine1)[ld.off + i].length;
				}
			}

			linesConvergence = linesConvergence * 100 / pLine1->size();
			linesLenConvergence = linesLenConvergence * 100 / line1Len;

			// Take the better convergence of the two
			if (linesConvergence < linesLenConvergence)
				linesConvergence = linesLenConvergence;

			if (linesConvergence >= 50)
				orderedLinesConvergence.emplace(conv_key(linesConvergence, line1, line2));
		}
	}

	std::map<int, std::pair<int, int>> bestLineMappings;
	int bestBlockConvergence = 0;

	for (auto startItr = orderedLinesConvergence.begin(); startItr != orderedLinesConvergence.end(); ++startItr)
	{
		std::map<int, std::pair<int, int>> lineMappings;

		std::vector<bool> mappedLines1(linesCount1, false);
		std::vector<bool> mappedLines2(linesCount2, false);

		int mappedLinesCount1 = 0;
		int mappedLinesCount2 = 0;

		for (auto ocItr = startItr; ocItr != orderedLinesConvergence.end(); ++ocItr)
		{
			if (!mappedLines1[ocItr->line1] && !mappedLines2[ocItr->line2])
			{
				lineMappings.emplace(ocItr->line1, std::pair<int, int>(ocItr->convergence, ocItr->line2));

				if ((++mappedLinesCount1 == linesCount1) || (++mappedLinesCount2 == linesCount2))
					break;

				mappedLines1[ocItr->line1] = true;
				mappedLines2[ocItr->line2] = true;
			}
		}

		int currentBlockConvergence = 0;
		int lastLine2 = -1;

		for (const auto& lm: lineMappings)
		{
			// lines1 are stored in ascending order and to have a match lines2 must also be in ascending order
			if (lm.second.second > lastLine2)
			{
				currentBlockConvergence += lm.second.first;
				lastLine2 = lm.second.second;
			}
		}

		if (bestBlockConvergence < currentBlockConvergence)
		{
			bestBlockConvergence = currentBlockConvergence;
			bestLineMappings = std::move(lineMappings);
		}
	}

	if (!bestLineMappings.empty())
		compareLines(doc1, doc2, options, blockDiff1, blockDiff2, chunk1, chunk2, bestLineMappings);

	return;
}


void markSection(const diffInfo& bd, const DocCmpInfo& doc)
{
	const int endOff = doc.section.off + doc.section.len;

	for (int i = doc.section.off, line = bd.off + doc.section.off; i < endOff; ++i, ++line)
	{
		int movedLen = bd.info.movedSection(i);

		if (movedLen > doc.section.len)
			movedLen = doc.section.len;

		if (movedLen == 0)
		{
			int mark = doc.blockDiffMask;

			if (doc.nonUniqueLines.find(line) != doc.nonUniqueLines.end())
				mark = (mark == MARKER_MASK_ADDED) ? MARKER_MASK_ADDED_LOCAL : MARKER_MASK_REMOVED_LOCAL;

			CallScintilla(doc.view, SCI_MARKERADDSET, line, mark);
		}
		else if (movedLen == 1)
		{
			CallScintilla(doc.view, SCI_MARKERADDSET, line, MARKER_MASK_MOVED_LINE);
		}
		else
		{
			i += --movedLen;

			CallScintilla(doc.view, SCI_MARKERADDSET, line, MARKER_MASK_MOVED_BEGIN);

			for (--movedLen, ++line; movedLen; --movedLen, ++line)
				CallScintilla(doc.view, SCI_MARKERADDSET, line, MARKER_MASK_MOVED_MID);

			CallScintilla(doc.view, SCI_MARKERADDSET, line, MARKER_MASK_MOVED_END);
		}
	}
}


void markLineDiffs(const CompareInfo& cmpInfo, const diffInfo& bd, int lineIdx)
{
	int line = bd.off + bd.info.changedLines[lineIdx].line;
	int linePos = CallScintilla(cmpInfo.doc1.view, SCI_POSITIONFROMLINE, line, 0);

	for (const auto& change: bd.info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc1.view, linePos + change.off, change.len);

	CallScintilla(cmpInfo.doc1.view, SCI_MARKERADDSET, line,
			cmpInfo.doc1.nonUniqueLines.find(line) == cmpInfo.doc1.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);

	line = bd.info.matchBlock->off + bd.info.matchBlock->info.changedLines[lineIdx].line;
	linePos = CallScintilla(cmpInfo.doc2.view, SCI_POSITIONFROMLINE, line, 0);

	for (const auto& change: bd.info.matchBlock->info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc2.view, linePos + change.off, change.len);

	CallScintilla(cmpInfo.doc2.view, SCI_MARKERADDSET, line,
			cmpInfo.doc2.nonUniqueLines.find(line) == cmpInfo.doc2.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);
}


bool markAllDiffs(CompareInfo& cmpInfo, AlignmentInfo_t& alignmentInfo)
{
	progress_ptr& progress = ProgressDlg::Get();

	alignmentInfo.clear();

	const int blockDiffSize = static_cast<int>(cmpInfo.blockDiffs.size());

	if (progress)
		progress->SetMaxCount(blockDiffSize);

	AlignmentPair alignPair;

	AlignmentViewData* pMainAlignData	= &alignPair.main;
	AlignmentViewData* pSubAlignData	= &alignPair.sub;

	// Make sure pMainAlignData is linked to doc1
	if (cmpInfo.doc1.view == SUB_VIEW)
		std::swap(pMainAlignData, pSubAlignData);

	pMainAlignData->line	= cmpInfo.doc1.section.off;
	pSubAlignData->line		= cmpInfo.doc2.section.off;

	for (int i = 0; i < blockDiffSize; ++i)
	{
		const diffInfo& bd = cmpInfo.blockDiffs[i];

		if (bd.type == diff_type::DIFF_MATCH)
		{
			pMainAlignData->diffMask	= 0;
			pSubAlignData->diffMask		= 0;

			alignmentInfo.push_back(alignPair);

			pMainAlignData->line	+= bd.len;
			pSubAlignData->line		+= bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_2)
		{
			cmpInfo.doc2.section.off = 0;
			cmpInfo.doc2.section.len = bd.len;
			markSection(bd, cmpInfo.doc2);

			pMainAlignData->diffMask	= 0;
			pSubAlignData->diffMask		= cmpInfo.doc2.blockDiffMask;

			alignmentInfo.push_back(alignPair);

			pSubAlignData->line += bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_1)
		{
			if (bd.info.matchBlock)
			{
				const int changedLinesCount = static_cast<int>(bd.info.changedLines.size());

				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc2.section.off = 0;

				for (int j = 0; j < changedLinesCount; ++j)
				{
					cmpInfo.doc1.section.len = bd.info.changedLines[j].line - cmpInfo.doc1.section.off;
					cmpInfo.doc2.section.len = bd.info.matchBlock->info.changedLines[j].line - cmpInfo.doc2.section.off;

					if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
					{
						pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
						pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;

						alignmentInfo.push_back(alignPair);

						if (cmpInfo.doc1.section.len)
						{
							markSection(bd, cmpInfo.doc1);
							pMainAlignData->line += cmpInfo.doc1.section.len;
						}

						if (cmpInfo.doc2.section.len)
						{
							markSection(*bd.info.matchBlock, cmpInfo.doc2);
							pSubAlignData->line += cmpInfo.doc2.section.len;
						}
					}

					pMainAlignData->diffMask	= MARKER_MASK_CHANGED;
					pSubAlignData->diffMask		= MARKER_MASK_CHANGED;

					alignmentInfo.push_back(alignPair);

					markLineDiffs(cmpInfo, bd, j);

					cmpInfo.doc1.section.off = bd.info.changedLines[j].line + 1;
					cmpInfo.doc2.section.off = bd.info.matchBlock->info.changedLines[j].line + 1;

					++(pMainAlignData->line);
					++(pSubAlignData->line);
				}

				cmpInfo.doc1.section.len = bd.len - cmpInfo.doc1.section.off;
				cmpInfo.doc2.section.len = bd.info.matchBlock->len - cmpInfo.doc2.section.off;

				if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
				{
					pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
					pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;

					alignmentInfo.push_back(alignPair);

					if (cmpInfo.doc1.section.len)
					{
						markSection(bd, cmpInfo.doc1);
						pMainAlignData->line += cmpInfo.doc1.section.len;
					}

					if (cmpInfo.doc2.section.len)
					{
						markSection(*bd.info.matchBlock, cmpInfo.doc2);
						pSubAlignData->line += cmpInfo.doc2.section.len;
					}
				}

				++i;
			}
			else
			{
				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc1.section.len = bd.len;
				markSection(bd, cmpInfo.doc1);

				pMainAlignData->diffMask	= cmpInfo.doc1.blockDiffMask;
				pSubAlignData->diffMask		= 0;

				alignmentInfo.push_back(alignPair);

				pMainAlignData->line += bd.len;
			}
		}

		if (progress && !progress->Advance())
			return false;
	}

	if (cmpInfo.selectionCompare)
	{
		pMainAlignData->diffMask	= 0;
		pSubAlignData->diffMask		= 0;

		alignmentInfo.push_back(alignPair);
	}

	if (progress && !progress->NextPhase())
		return false;

	return true;
}


CompareResult runCompare(const CompareOptions& options, AlignmentInfo_t& alignmentInfo)
{
	progress_ptr& progress = ProgressDlg::Get();

	CompareInfo cmpInfo;

	cmpInfo.doc1.view	= MAIN_VIEW;
	cmpInfo.doc2.view	= SUB_VIEW;

	cmpInfo.selectionCompare	= options.selectionCompare;

	if (options.selectionCompare)
	{
		cmpInfo.doc1.section.off	= options.selections[MAIN_VIEW].first;
		cmpInfo.doc1.section.len	= options.selections[MAIN_VIEW].second - options.selections[MAIN_VIEW].first + 1;

		cmpInfo.doc2.section.off	= options.selections[SUB_VIEW].first;
		cmpInfo.doc2.section.len	= options.selections[SUB_VIEW].second - options.selections[SUB_VIEW].first + 1;
	}

	cmpInfo.doc1.blockDiffMask = (options.oldFileViewId == MAIN_VIEW) ? MARKER_MASK_REMOVED : MARKER_MASK_ADDED;
	cmpInfo.doc2.blockDiffMask = (options.oldFileViewId == MAIN_VIEW) ? MARKER_MASK_ADDED : MARKER_MASK_REMOVED;

	const std::vector<uint64_t> doc1LineHashes = computeLineHashes(cmpInfo.doc1, options);

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	const std::vector<uint64_t> doc2LineHashes = computeLineHashes(cmpInfo.doc2, options);

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	const std::vector<uint64_t>* pLineHashes1 = &doc1LineHashes;
	const std::vector<uint64_t>* pLineHashes2 = &doc2LineHashes;

	if (pLineHashes1->size() < pLineHashes2->size())
	{
		std::swap(pLineHashes1, pLineHashes2);
		std::swap(cmpInfo.doc1, cmpInfo.doc2);
	}

	cmpInfo.blockDiffs = DiffCalc<uint64_t, blockDiffInfo>(*pLineHashes1, *pLineHashes2)();

	const int blockDiffSize = static_cast<int>(cmpInfo.blockDiffs.size());

	if (blockDiffSize == 0 || (blockDiffSize == 1 && cmpInfo.blockDiffs[0].type == diff_type::DIFF_MATCH))
		return CompareResult::COMPARE_MATCH;

	findUniqueLines(cmpInfo, *pLineHashes1, *pLineHashes2);

	if (options.detectMoves)
		findMoves(cmpInfo, *pLineHashes1, *pLineHashes2);

	if (cmpInfo.doc1.section.off || cmpInfo.doc2.section.off)
	{
		for (auto& bd: cmpInfo.blockDiffs)
		{
			if (bd.type == diff_type::DIFF_IN_1 || bd.type == diff_type::DIFF_MATCH)
				bd.off += cmpInfo.doc1.section.off;
			else if (bd.type == diff_type::DIFF_IN_2)
				bd.off += cmpInfo.doc2.section.off;
		}
	}

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	if (progress)
		progress->SetMaxCount(blockDiffSize);

	// Do block compares
	for (int i = 0; i < blockDiffSize; ++i)
	{
		if (cmpInfo.blockDiffs[i].type == diff_type::DIFF_IN_2)
		{
			// Check if the DIFF_IN_1 / DIFF_IN_2 pair includes changed lines or it's a completely replaced block
			if (i != 0 && cmpInfo.blockDiffs[i - 1].type == diff_type::DIFF_IN_1)
			{
				diffInfo& blockDiff1 = cmpInfo.blockDiffs[i - 1];
				diffInfo& blockDiff2 = cmpInfo.blockDiffs[i];

				blockDiff1.info.matchBlock = &blockDiff2;
				blockDiff2.info.matchBlock = &blockDiff1;

				compareBlocks(cmpInfo.doc1, cmpInfo.doc2, options, blockDiff1, blockDiff2);
			}
		}

		if (progress && !progress->Advance())
			return CompareResult::COMPARE_CANCELLED;
	}

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	if (!markAllDiffs(cmpInfo, alignmentInfo))
		return CompareResult::COMPARE_CANCELLED;

	return CompareResult::COMPARE_MISMATCH;
}


CompareResult runFindUnique(const CompareOptions& options, AlignmentInfo_t& alignmentInfo)
{
	progress_ptr& progress = ProgressDlg::Get();

	alignmentInfo.clear();

	DocCmpInfo doc1;
	DocCmpInfo doc2;

	doc1.view	= MAIN_VIEW;
	doc2.view	= SUB_VIEW;

	if (options.selectionCompare)
	{
		doc1.section.off	= options.selections[MAIN_VIEW].first;
		doc1.section.len	= options.selections[MAIN_VIEW].second - options.selections[MAIN_VIEW].first + 1;

		doc2.section.off	= options.selections[SUB_VIEW].first;
		doc2.section.len	= options.selections[SUB_VIEW].second - options.selections[SUB_VIEW].first + 1;
	}

	if (options.oldFileViewId == MAIN_VIEW)
	{
		doc1.blockDiffMask = MARKER_MASK_REMOVED;
		doc2.blockDiffMask = MARKER_MASK_ADDED;
	}
	else
	{
		doc1.blockDiffMask = MARKER_MASK_ADDED;
		doc2.blockDiffMask = MARKER_MASK_REMOVED;
	}

	std::vector<uint64_t> doc1LineHashes = computeLineHashes(doc1, options);

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::vector<uint64_t> doc2LineHashes = computeLineHashes(doc2, options);

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<int>> doc1UniqueLines;

	int docHashesSize = static_cast<int>(doc1LineHashes.size());

	for (int i = 0; i < docHashesSize; ++i)
	{
		auto insertPair = doc1UniqueLines.emplace(doc1LineHashes[i], std::vector<int>{i});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(i);
	}

	doc1LineHashes.clear();

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<int>> doc2UniqueLines;

	docHashesSize = static_cast<int>(doc2LineHashes.size());

	for (int i = 0; i < docHashesSize; ++i)
	{
		auto insertPair = doc2UniqueLines.emplace(doc2LineHashes[i], std::vector<int>{i});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(i);
	}

	doc2LineHashes.clear();

	if (progress && !progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	int doc1UniqueLinesCount = 0;

	for (std::unordered_map<uint64_t, std::vector<int>>::iterator doc1it = doc1UniqueLines.begin();
		doc1it != doc1UniqueLines.end(); ++doc1it)
	{
		std::unordered_map<uint64_t, std::vector<int>>::iterator doc2it = doc2UniqueLines.find(doc1it->first);

		if (doc2it != doc2UniqueLines.end())
		{
			doc2UniqueLines.erase(doc2it);
		}
		else
		{
			for (const auto& line: doc1it->second)
			{
				CallScintilla(doc1.view, SCI_MARKERADDSET, line + doc1.section.off, doc1.blockDiffMask);
				++doc1UniqueLinesCount;
			}
		}
	}

	if (doc1UniqueLinesCount == 0 && doc2UniqueLines.empty())
		return CompareResult::COMPARE_MATCH;

	for (const auto& uniqueLine: doc2UniqueLines)
	{
		for (const auto& line: uniqueLine.second)
		{
			CallScintilla(doc2.view, SCI_MARKERADDSET, line + doc2.section.off, doc2.blockDiffMask);
		}
	}

	AlignmentPair align;
	align.main.line	= doc1.section.off;
	align.sub.line	= doc2.section.off;

	alignmentInfo.push_back(align);

	return CompareResult::COMPARE_MISMATCH;
}

}


CompareResult compareViews(const CompareOptions& options, const TCHAR* progressInfo, AlignmentInfo_t& alignmentInfo)
{
	CompareResult result = CompareResult::COMPARE_ERROR;

	if (progressInfo)
		ProgressDlg::Open(progressInfo);

	try
	{
		if (options.findUniqueMode)
			result = runFindUnique(options, alignmentInfo);
		else
			result = runCompare(options, alignmentInfo);

		ProgressDlg::Close();
	}
	catch (std::exception& e)
	{
		ProgressDlg::Close();

		char msg[128];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
		::MessageBoxA(nppData._nppHandle, msg, "Compare", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		ProgressDlg::Close();

		::MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "Compare", MB_OK | MB_ICONWARNING);
	}

	return result;
}
