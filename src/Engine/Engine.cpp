/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2022 Pavel Nedev (pg.nedev@gmail.com)
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

#define NOMINMAX

#include <climits>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>

#include <windows.h>

#include "Engine.h"
#include "diff.h"
#include "ProgressDlg.h"

#define MULTITHREAD		0

#if defined(MULTITHREAD) && (MULTITHREAD != 0)

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#include "../mingw-std-threads/mingw.mutex.h"
#else
#include <thread>
#include <mutex>
#endif // __MINGW32__ ...

#endif // MULTITHREAD


namespace {

enum class charType
{
	SPACECHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


struct Line
{
	intptr_t line;

	uint64_t hash;

	inline bool operator==(const Line& rhs) const
	{
		return (hash == rhs.hash);
	}

	inline bool operator!=(const Line& rhs) const
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


struct Word
{
	intptr_t pos;
	intptr_t len;

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


struct Char
{
	Char(wchar_t c, intptr_t p) : ch(c), pos(p) {}

	wchar_t ch;
	intptr_t pos;

	inline bool operator==(const Char& rhs) const
	{
		return (ch == rhs.ch);
	}

	inline bool operator!=(const Char& rhs) const
	{
		return (ch != rhs.ch);
	}

	inline bool operator==(wchar_t rhs) const
	{
		return (ch == rhs);
	}

	inline bool operator!=(wchar_t rhs) const
	{
		return (ch != rhs);
	}
};


struct DocCmpInfo
{
	int			view;
	section_t	section;

	int			blockDiffMask;

	std::vector<Line>				lines;
	std::unordered_set<intptr_t>	nonUniqueLines;
};


struct diffLine
{
	diffLine(intptr_t lineNum) : line(lineNum) {}

	intptr_t line;
	std::vector<section_t> changes;
};


struct blockDiffInfo
{
	const diff_info<blockDiffInfo>*	matchBlock {nullptr};

	std::vector<diffLine>	changedLines;
	std::vector<section_t>	moves;

	inline intptr_t movedCount() const
	{
		intptr_t count = 0;

		for (const auto& move: moves)
			count += move.len;

		return count;
	}

	inline intptr_t movedSection(intptr_t line) const
	{
		for (const auto& move: moves)
		{
			if (line >= move.off && line < move.off + move.len)
				return move.len;
		}

		return 0;
	}

	inline bool getNextUnmoved(intptr_t& line) const
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

	// Output data - filled by the compare engine
	std::vector<diffInfo>	blockDiffs;
};


struct MatchInfo
{
	intptr_t	lookupOff;
	diffInfo*	matchDiff;
	intptr_t	matchOff;
	intptr_t	matchLen;
};


struct Conv
{
	float		convergence;
	intptr_t	diffsCount;

	Conv(float c = 0, intptr_t dc = 0) : convergence(c), diffsCount(dc)
	{}

	Conv(const Conv& c) : convergence(c.convergence), diffsCount(c.diffsCount)
	{}

	const Conv& operator=(const Conv& rhs)
	{
		if (&rhs != this)
			Set(rhs.convergence, rhs.diffsCount);

		return *this;
	}

	void Set(float c, intptr_t dc)
	{
		convergence = c;
		diffsCount = dc;
	}

	bool operator>(const Conv& rhs) const
	{
		return ((diffsCount < rhs.diffsCount) || ((diffsCount == rhs.diffsCount) && (convergence > rhs.convergence)));
	}

	bool operator==(const Conv& rhs) const
	{
		return ((diffsCount == rhs.diffsCount) && (convergence == rhs.convergence));
	}
};


struct LinesConv
{
	Conv conv;

	intptr_t line1;
	intptr_t line2;

	LinesConv() : line1(-1), line2(-1)
	{}

	LinesConv(const Conv& c, intptr_t l1, intptr_t l2) : conv(c), line1(l1), line2(l2)
	{}

	void Set(const Conv& c, intptr_t l1, intptr_t l2)
	{
		conv = c;
		line1 = l1;
		line2 = l2;
	}

	bool operator<(const LinesConv& rhs) const
	{
		return ((conv > rhs.conv) || ((conv == rhs.conv) && (line2 < rhs.line2)));
	}
};


const uint64_t cHashSeed = 0x84222325;

template<typename CharT>
inline uint64_t Hash(uint64_t hval, CharT letter)
{
	hval ^= static_cast<uint64_t>(letter);

	hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);

	return hval;
}


inline intptr_t toAlignmentLine(const DocCmpInfo& doc, intptr_t bdLine)
{
	if (doc.lines.empty())
		return 0;
	else if (bdLine < 0)
		return doc.lines.front().line;
	else if (bdLine < (intptr_t)doc.lines.size())
		return doc.lines[bdLine].line;

	return (doc.lines.back().line + 1);
}


void swap(DocCmpInfo& lhs, DocCmpInfo& rhs)
{
	std::swap(lhs.view, rhs.view);
	std::swap(lhs.section, rhs.section);
	std::swap(lhs.blockDiffMask, rhs.blockDiffMask);
	std::swap(lhs.lines, rhs.lines);
	std::swap(lhs.nonUniqueLines, rhs.nonUniqueLines);
}


inline uint64_t lineRangeHash(uint64_t hashSeed, std::vector<wchar_t>& line, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos < endPos)
	{
		if (options.ignoreCase)
		{
			const wchar_t storedChar = line[endPos];

			line[endPos] = L'\0';

			::CharLowerW((LPWSTR)line.data() + pos);

			line[endPos] = storedChar;
		}

		for (; pos < endPos; ++pos)
		{
			if (options.ignoreSpaces && (line[pos] == L' ' || line[pos] == L'\t'))
				continue;

			hashSeed = Hash(hashSeed, line[pos]);
		}
	}

	return hashSeed;
}


uint64_t regexIgnoreLineHash(uint64_t hashSeed, int codepage, const std::vector<char>& line,
	const CompareOptions& options)
{
	const int len = static_cast<int>(line.size());

	if (len == 0)
		return hashSeed;

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

#if !defined(MULTITHREAD) || (MULTITHREAD == 0)
	LOGD(LOG_ALGO, "line len " + std::to_string(len) + " to wide char len " + std::to_string(wLen) + "\n");
#endif

	std::regex_iterator<std::vector<wchar_t>::iterator> rit(wLine.begin(), wLine.end(), *options.ignoreRegex);
	std::regex_iterator<std::vector<wchar_t>::iterator> rend;

	intptr_t pos = 0;

	while (rit != rend)
	{
#if !defined(MULTITHREAD) || (MULTITHREAD == 0)
		LOGD(LOG_ALGO, "pos " + std::to_string(rit->position()) + ", len " + std::to_string(rit->length()) + "\n");
#endif

		hashSeed = lineRangeHash(hashSeed, wLine, pos, rit->position(), options);

		pos = rit->position() + rit->length();
		++rit;
	}

	hashSeed = lineRangeHash(hashSeed, wLine, pos, wLen - 1, options);

	return hashSeed;
}


void getLines(DocCmpInfo& doc, const CompareOptions& options)
{
	const int monitorCancelEveryXLine = 500;

	progress_ptr& progress = ProgressDlg::Get();

	doc.lines.clear();

	intptr_t linesCount = CallScintilla(doc.view, SCI_GETLENGTH, 0, 0);

	if (linesCount)
		linesCount = CallScintilla(doc.view, SCI_GETLINECOUNT, 0, 0);
	else
		return;

	if ((doc.section.len <= 0) || (doc.section.off + doc.section.len > linesCount))
		doc.section.len = linesCount - doc.section.off;

	progress->SetMaxCount((doc.section.len / monitorCancelEveryXLine) + 1);

	doc.lines.reserve(doc.section.len);

	for (intptr_t secLine = 0; secLine < doc.section.len; ++secLine)
	{
		if ((secLine % monitorCancelEveryXLine == 0) && !progress->Advance())
		{
			doc.lines.clear();
			return;
		}

		const int codepage			= getCodepage(doc.view);

		const intptr_t docLine		= secLine + doc.section.off;
		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEnd		= getLineEnd(doc.view, docLine);

		Line newLine;
		newLine.hash = cHashSeed;
		newLine.line = docLine;

		if (lineStart < lineEnd)
		{
			std::vector<char> line = getText(doc.view, lineStart, lineEnd);

			if (options.ignoreRegex)
			{
#if !defined(MULTITHREAD) || (MULTITHREAD == 0)
				LOGD(LOG_ALGO, "Regex Ignore on line " + std::to_string(docLine + 1) +
						", view " + std::to_string(doc.view) + "\n");
#endif

				newLine.hash = regexIgnoreLineHash(newLine.hash, codepage, line, options);
			}
			else
			{
				if (options.ignoreCase)
					toLowerCase(line, codepage);

				for (intptr_t i = 0; i < lineEnd - lineStart; ++i)
				{
					if (options.ignoreSpaces && (line[i] == ' ' || line[i] == '\t'))
						continue;

					newLine.hash = Hash(newLine.hash, line[i]);
				}
			}
		}

		if (!options.ignoreEmptyLines || newLine.hash != cHashSeed)
			doc.lines.emplace_back(newLine);
	}
}


charType getCharTypeW(wchar_t letter)
{
	if (letter == L' ' || letter == L'\t')
		return charType::SPACECHAR;

	if (::IsCharAlphaNumericW(letter) || letter == L'_')
		return charType::ALPHANUMCHAR;

	return charType::OTHERCHAR;
}


inline void recalculateWordPos(int codepage, std::vector<Word>& words, const std::vector<wchar_t>& line)
{
	intptr_t bytePos = 0;
	intptr_t currPos = 0;

	for (auto& word : words)
	{
		if (currPos < word.pos)
			bytePos += ::WideCharToMultiByte(codepage, 0, line.data() + currPos, static_cast<int>(word.pos - currPos),
					NULL, 0, NULL, NULL);

		currPos = word.pos + word.len;
		word.len = ::WideCharToMultiByte(codepage, 0, line.data() + word.pos, static_cast<int>(word.len),
				NULL, 0, NULL, NULL);
		word.pos = bytePos;
		bytePos += word.len;
	}
}


inline void getLineRangeWords(std::vector<Word>& words, std::vector<wchar_t>& line, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos < endPos)
	{
		if (options.ignoreCase)
		{
			const wchar_t storedChar = line[endPos];

			line[endPos] = L'\0';

			::CharLowerW((LPWSTR)line.data() + pos);

			line[endPos] = storedChar;
		}

		charType currentWordType = getCharTypeW(line[pos]);

		Word word;
		word.hash = Hash(cHashSeed, line[pos]);
		word.pos = pos;
		word.len = 1;

		for (++pos; pos < endPos; ++pos)
		{
			charType newWordType = getCharTypeW(line[pos]);

			if (newWordType == currentWordType)
			{
				++word.len;
				word.hash = Hash(word.hash, line[pos]);
			}
			else
			{
				if (!options.ignoreSpaces || currentWordType != charType::SPACECHAR)
					words.emplace_back(word);

				currentWordType = newWordType;

				word.hash = Hash(cHashSeed, line[pos]);
				word.pos = pos;
				word.len = 1;
			}
		}

		if (!options.ignoreSpaces || currentWordType != charType::SPACECHAR)
			words.emplace_back(word);
	}
}


std::vector<Word> getRegexIgnoreLineWords(std::vector<wchar_t>& line, const CompareOptions& options)
{
	std::vector<Word> words;

	const intptr_t len = static_cast<intptr_t>(line.size());

	if (len == 0)
		return words;

	std::regex_iterator<std::vector<wchar_t>::iterator> rit(line.begin(), line.end(), *options.ignoreRegex);
	std::regex_iterator<std::vector<wchar_t>::iterator> rend;

	intptr_t pos = 0;

	while (rit != rend)
	{
		getLineRangeWords(words, line, pos, rit->position(), options);

		pos = rit->position() + rit->length();
		++rit;
	}

	getLineRangeWords(words, line, pos, len - 1, options);

	return words;
}


std::vector<Word> getLineWords(int view, intptr_t docLine, const CompareOptions& options)
{
	std::vector<Word> words;

	const int codepage			= getCodepage(view);

	const intptr_t lineStart	= getLineStart(view, docLine);
	const intptr_t lineEnd		= getLineEnd(view, docLine);

	if (lineStart < lineEnd)
	{
		std::vector<char> line = getText(view, lineStart, lineEnd);

		const int len = static_cast<int>(line.size());

		const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

		std::vector<wchar_t> wLine(wLen);

		::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

		if (options.ignoreRegex)
			words = getRegexIgnoreLineWords(wLine, options);
		else
			getLineRangeWords(words, wLine, 0, wLen - 1, options);

		// In case of UTF-16 or UTF-32 find words byte positions and lengths because Scintilla uses those
		if (wLen != len)
			recalculateWordPos(codepage, words, wLine);
	}

	return words;
}


inline void recalculateCharPos(int codepage, std::vector<Char>& chars, const std::vector<wchar_t>& sec)
{
	intptr_t bytePos = 0;
	intptr_t currPos = 0;

	for (auto& ch : chars)
	{
		if (currPos < ch.pos)
			bytePos += ::WideCharToMultiByte(codepage, 0, sec.data() + currPos, static_cast<int>(ch.pos - currPos),
					NULL, 0, NULL, NULL);

		currPos = ch.pos + 1;
		const int charLen = ::WideCharToMultiByte(codepage, 0, sec.data() + ch.pos, 1, NULL, 0, NULL, NULL);
		ch.pos = bytePos;
		bytePos += charLen;
	}
}


void getSectionRangeChars(std::vector<Char>& chars, std::vector<wchar_t>& sec, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos < endPos)
	{
		if (options.ignoreCase)
		{
			const wchar_t storedChar = sec[endPos];

			sec[endPos] = L'\0';

			::CharLowerW((LPWSTR)sec.data() + pos);

			sec[endPos] = storedChar;
		}

		for (; pos < endPos; ++pos)
		{
			if (!options.ignoreSpaces || getCharTypeW(sec[pos]) != charType::SPACECHAR)
				chars.emplace_back(sec[pos], pos);
		}
	}
}


std::vector<Char> getSectionChars(int view, intptr_t secStart, intptr_t secEnd, const CompareOptions& options)
{
	std::vector<Char> chars;

	if (secStart < secEnd)
	{
		const int codepage = getCodepage(view);

		std::vector<char> sec = getText(view, secStart, secEnd);

		const int len = static_cast<int>(sec.size());

		const int wLen = ::MultiByteToWideChar(codepage, 0, sec.data(), len, NULL, 0);

		std::vector<wchar_t> wSec(wLen);

		::MultiByteToWideChar(codepage, 0, sec.data(), len, wSec.data(), wLen);

		chars.reserve(wLen - 1);

		getSectionRangeChars(chars, wSec, 0, wLen - 1, options);

		// In case of UTF-16 or UTF-32 find chars byte positions because Scintilla uses those
		if (wLen != len)
			recalculateCharPos(codepage, chars, wSec);
	}

	return chars;
}


std::vector<Char> getRegexIgnoreChars(int view, intptr_t secStart, intptr_t secEnd, const CompareOptions& options)
{
	std::vector<Char> chars;

	const int codepage = getCodepage(view);

	if (secStart < secEnd)
	{
		std::vector<char> sec = getText(view, secStart, secEnd);

		const int len = static_cast<int>(sec.size());

		const int wLen = ::MultiByteToWideChar(codepage, 0, sec.data(), len, NULL, 0);

		std::vector<wchar_t> wSec(wLen);

		::MultiByteToWideChar(codepage, 0, sec.data(), len, wSec.data(), wLen);

		chars.reserve(wLen - 1);

		std::regex_iterator<std::vector<wchar_t>::iterator> rit(wSec.begin(), wSec.end(), *options.ignoreRegex);
		std::regex_iterator<std::vector<wchar_t>::iterator> rend;

		intptr_t pos = 0;

		while (rit != rend)
		{
			getSectionRangeChars(chars, wSec, pos, rit->position(), options);

			pos = rit->position() + rit->length();
			++rit;
		}

		getSectionRangeChars(chars, wSec, pos, wLen - 1, options);

		// In case of UTF-16 or UTF-32 find chars byte positions because Scintilla uses those
		if (wLen != len)
			recalculateCharPos(codepage, chars, wSec);
	}

	return chars;
}


std::vector<std::vector<Char>> getChars(const DocCmpInfo& doc, const diffInfo& blockDiff,
		const CompareOptions& options)
{
	std::vector<std::vector<Char>> chars(blockDiff.len);

	for (intptr_t blockLine = 0; blockLine < blockDiff.len; ++blockLine)
	{
		// Don't get moved lines
		if (blockDiff.info.getNextUnmoved(blockLine))
		{
			--blockLine;
			continue;
		}

		const intptr_t docLine		= doc.lines[blockLine + blockDiff.off].line;
		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEnd		= getLineEnd(doc.view, docLine);

		if (lineStart < lineEnd)
		{
			if (options.ignoreRegex)
				chars[blockLine] = getRegexIgnoreChars(doc.view, lineStart, lineEnd, options);
			else
				chars[blockLine] = getSectionChars(doc.view, lineStart, lineEnd, options);
		}
	}

	return chars;
}


// Scan for the best single matching block in the other file
void findBestMatch(const CompareInfo& cmpInfo, const diffInfo& lookupDiff, intptr_t lookupOff, MatchInfo& mi)
{
	mi.matchLen		= 0;
	mi.matchDiff	= nullptr;

	const std::vector<Line>* pLookupLines;
	const std::vector<Line>* pMatchLines;
	diff_type matchType;

	if (lookupDiff.type == diff_type::DIFF_IN_1)
	{
		pLookupLines	= &cmpInfo.doc1.lines;
		pMatchLines		= &cmpInfo.doc2.lines;
		matchType		= diff_type::DIFF_IN_2;
	}
	else
	{
		pLookupLines	= &cmpInfo.doc2.lines;
		pMatchLines		= &cmpInfo.doc1.lines;
		matchType		= diff_type::DIFF_IN_1;
	}

	intptr_t minMatchLen = 1;

	for (const diffInfo& matchDiff: cmpInfo.blockDiffs)
	{
		if (matchDiff.type != matchType || matchDiff.len < minMatchLen)
			continue;

		for (intptr_t matchOff = 0; matchOff < matchDiff.len; ++matchOff)
		{
			if ((*pLookupLines)[lookupDiff.off + lookupOff] != (*pMatchLines)[matchDiff.off + matchOff])
				continue;

			if (matchDiff.info.getNextUnmoved(matchOff))
			{
				if (matchOff >= matchDiff.len)
					break;

				if ((*pLookupLines)[lookupDiff.off + lookupOff] != (*pMatchLines)[matchDiff.off + matchOff])
					continue;
			}

			intptr_t lookupStart	= lookupOff - 1;
			intptr_t matchStart		= matchOff - 1;

			// Check for the beginning of the matched block (containing lookupOff element)
			for (; lookupStart >= 0 && matchStart >= 0 &&
					(*pLookupLines)[lookupDiff.off + lookupStart] == (*pMatchLines)[matchDiff.off + matchStart] &&
					!lookupDiff.info.movedSection(lookupStart) && !matchDiff.info.movedSection(matchStart);
					--lookupStart, --matchStart);

			++lookupStart;
			++matchStart;

			intptr_t lookupEnd	= lookupOff + 1;
			intptr_t matchEnd	= matchOff + 1;

			// Check for the end of the matched block (containing lookupOff element)
			for (; lookupEnd < lookupDiff.len && matchEnd < matchDiff.len &&
					(*pLookupLines)[lookupDiff.off + lookupEnd] == (*pMatchLines)[matchDiff.off + matchEnd] &&
					!lookupDiff.info.movedSection(lookupEnd) && !matchDiff.info.movedSection(matchEnd);
					++lookupEnd, ++matchEnd);

			const intptr_t matchLen = lookupEnd - lookupStart;

			if (mi.matchLen < matchLen)
			{
				mi.lookupOff	= lookupStart;
				mi.matchDiff	= const_cast<diffInfo*>(&matchDiff);
				mi.matchOff		= matchStart;
				mi.matchLen		= matchLen;

				minMatchLen		= matchLen;
				matchOff		= matchEnd - 1;
			}
			else if (mi.matchLen == matchLen)
			{
				mi.matchDiff	= nullptr;
				matchOff		= matchEnd - 1;
			}
		}
	}
}


// Recursively resolve the best match
bool resolveMatch(const CompareInfo& cmpInfo, diffInfo& lookupDiff, intptr_t lookupOff, MatchInfo& lookupMi)
{
	bool ret = false;

	if (lookupMi.matchDiff)
	{
		lookupOff = lookupMi.matchOff + (lookupOff - lookupMi.lookupOff);

		MatchInfo reverseMi;
		findBestMatch(cmpInfo, *(lookupMi.matchDiff), lookupOff, reverseMi);

		if ((reverseMi.matchDiff == &lookupDiff) && (reverseMi.matchOff == lookupMi.lookupOff))
		{
			LOGD(LOG_ALGO, "Move match found, len: " + std::to_string(lookupMi.matchLen) + "\n");

			lookupDiff.info.moves.emplace_back(lookupMi.lookupOff, lookupMi.matchLen);
			lookupMi.matchDiff->info.moves.emplace_back(lookupMi.matchOff, lookupMi.matchLen);
			ret = true;
		}
		else if (reverseMi.matchDiff)
		{
			ret = resolveMatch(cmpInfo, *(lookupMi.matchDiff), lookupOff, reverseMi);
			lookupMi.matchLen = 0;
		}
	}

	return ret;
}


void findMoves(CompareInfo& cmpInfo)
{
	LOGD(LOG_ALGO, "FIND MOVES\n");

	bool repeat = true;

	while (repeat)
	{
		repeat = false;

		for (diffInfo& lookupDiff: cmpInfo.blockDiffs)
		{
			if (lookupDiff.type != diff_type::DIFF_IN_1)
				continue;

			LOGD(LOG_ALGO, "Check D1 with off: " + std::to_string(cmpInfo.doc1.lines[lookupDiff.off].line + 1) + "\n");

			// Go through all lookupDiff's elements and check if each is matched
			for (intptr_t lookupEi = 0; lookupEi < lookupDiff.len; ++lookupEi)
			{
				// Skip already detected moves
				if (lookupDiff.info.getNextUnmoved(lookupEi))
				{
					if (lookupEi >= lookupDiff.len)
						break;
				}

				MatchInfo mi;
				findBestMatch(cmpInfo, lookupDiff, lookupEi, mi);

				if (resolveMatch(cmpInfo, lookupDiff, lookupEi, mi))
				{
					repeat = true;

					if (mi.matchLen)
						lookupEi = mi.lookupOff + mi.matchLen - 1;
					else
						--lookupEi;
				}
			}
		}
	}
}


void findUniqueLines(CompareInfo& cmpInfo)
{
	std::unordered_map<uint64_t, std::vector<intptr_t>> doc1LinesMap;

	for (const auto& line: cmpInfo.doc1.lines)
	{
		auto insertPair = doc1LinesMap.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	for (const auto& line: cmpInfo.doc2.lines)
	{
		auto doc1it = doc1LinesMap.find(line.hash);

		if (doc1it != doc1LinesMap.end())
		{
			cmpInfo.doc2.nonUniqueLines.emplace(line.line);

			auto insertPair = cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[0]);
			if (insertPair.second)
			{
				for (size_t j = 1; j < doc1it->second.size(); ++j)
					cmpInfo.doc1.nonUniqueLines.emplace(doc1it->second[j]);
			}
		}
	}
}


inline intptr_t matchBeginEnd(diffInfo& blockDiff1, diffInfo& blockDiff2,
		const std::vector<Char>& sec1, const std::vector<Char>& sec2,
		intptr_t off1, intptr_t off2, intptr_t end1, intptr_t end2,
		std::function<bool(const wchar_t)>&& charFilter_fn)
{
	const intptr_t minSecSize = std::min(sec1.size(), sec2.size());

	intptr_t startMatch = 0;
	while ((minSecSize > startMatch) && (sec1[startMatch] == sec2[startMatch]) && charFilter_fn(sec1[startMatch].ch))
		++startMatch;

	intptr_t endMatch = 0;
	while ((minSecSize - startMatch > endMatch) &&
			(sec1[sec1.size() - endMatch - 1] == sec2[sec2.size() - endMatch - 1]) &&
			charFilter_fn(sec1[sec1.size() - endMatch - 1].ch))
		++endMatch;

	if (startMatch || endMatch)
	{
		section_t change;

		if ((intptr_t)sec1.size() > startMatch + endMatch)
		{
			change.off = off1;
			if (startMatch)
				change.off += sec1[startMatch].pos;

			change.len = (endMatch ?
					sec1[sec1.size() - endMatch - 1].pos + 1 + off1 : end1) - change.off;

			if (change.len > 0)
				blockDiff1.info.changedLines.back().changes.emplace_back(change);
		}

		if ((intptr_t)sec2.size() > startMatch + endMatch)
		{
			change.off = off2;
			if (startMatch)
				change.off += sec2[startMatch].pos;

			change.len = (endMatch ?
					sec2[sec2.size() - endMatch - 1].pos + 1 + off2 : end2) - change.off;

			if (change.len > 0)
				blockDiff2.info.changedLines.back().changes.emplace_back(change);
		}
	}

	return (startMatch + endMatch);
}


void compareLines(const DocCmpInfo& doc1, const DocCmpInfo& doc2, diffInfo& blockDiff1, diffInfo& blockDiff2,
		const std::map<intptr_t, intptr_t>& lineMappings, const CompareOptions& options)
{
	for (const auto& lm: lineMappings)
	{
		intptr_t line1 = lm.second;
		intptr_t line2 = lm.first;

		LOGD(LOG_ALGO, "Compare Lines " + std::to_string(doc1.lines[blockDiff1.off + line1].line + 1) + " and " +
				std::to_string(doc2.lines[blockDiff2.off + line2].line + 1) + "\n");

		const std::vector<Word> lineWords1 = getLineWords(doc1.view, doc1.lines[blockDiff1.off + line1].line, options);
		const std::vector<Word> lineWords2 = getLineWords(doc2.view, doc2.lines[blockDiff2.off + line2].line, options);

		const auto* pLine1 = &lineWords1;
		const auto* pLine2 = &lineWords2;

		const DocCmpInfo* pDoc1 = &doc1;
		const DocCmpInfo* pDoc2 = &doc2;

		diffInfo* pBlockDiff1 = &blockDiff1;
		diffInfo* pBlockDiff2 = &blockDiff2;

		// First use word granularity (find matching words) for better precision
		auto wordDiffRes = DiffCalc<Word>(lineWords1, lineWords2)(!options.detectCharDiffs, true);
		const std::vector<diff_info<void>> lineDiffs = std::move(wordDiffRes.first);

		if (wordDiffRes.second)
		{
			std::swap(pDoc1, pDoc2);
			std::swap(pBlockDiff1, pBlockDiff2);
			std::swap(pLine1, pLine2);
			std::swap(line1, line2);
		}

		const intptr_t lineDiffsSize = static_cast<intptr_t>(lineDiffs.size());

		PRINT_DIFFS("WORD DIFFS", lineDiffs);

		pBlockDiff1->info.changedLines.emplace_back(line1);
		pBlockDiff2->info.changedLines.emplace_back(line2);

		const intptr_t lineOff1 = getLineStart(pDoc1->view, pDoc1->lines[line1 + pBlockDiff1->off].line);
		const intptr_t lineOff2 = getLineStart(pDoc2->view, pDoc2->lines[line2 + pBlockDiff2->off].line);

		intptr_t lineLen1 = 0;
		intptr_t lineLen2 = 0;

		for (const auto& word: *pLine1)
			lineLen1 += word.len;

		for (const auto& word: *pLine2)
			lineLen2 += word.len;

		intptr_t totalLineMatchLen = 0;

		for (intptr_t i = 0; i < lineDiffsSize; ++i)
		{
			const auto& ld = lineDiffs[i];

			if (ld.type == diff_type::DIFF_MATCH)
			{
				for (intptr_t j = 0; j < ld.len; ++j)
					totalLineMatchLen += (*pLine1)[ld.off + j].len;
			}
			else if (ld.type == diff_type::DIFF_IN_2)
			{
				section_t change;

				change.off = (*pLine2)[ld.off].pos;
				change.len = (*pLine2)[ld.off + ld.len - 1].pos + (*pLine2)[ld.off + ld.len - 1].len - change.off;

				pBlockDiff2->info.changedLines.back().changes.emplace_back(change);
			}
			else
			{
				// Resolve words mismatched DIFF_IN_1 / DIFF_IN_2 pairs to find possible sub-word similarities
				if ((i + 1 < lineDiffsSize) && (lineDiffs[i + 1].type == diff_type::DIFF_IN_2))
				{
					const auto& ld2 = lineDiffs[i + 1];

					intptr_t off1 = (*pLine1)[ld.off].pos;
					intptr_t end1 = (*pLine1)[ld.off + ld.len - 1].pos + (*pLine1)[ld.off + ld.len - 1].len;

					intptr_t off2 = (*pLine2)[ld2.off].pos;
					intptr_t end2 = (*pLine2)[ld2.off + ld2.len - 1].pos + (*pLine2)[ld2.off + ld2.len - 1].len;

					const std::vector<Char> sec1 =
							getSectionChars(pDoc1->view, off1 + lineOff1, end1 + lineOff1, options);
					const std::vector<Char> sec2 =
							getSectionChars(pDoc2->view, off2 + lineOff2, end2 + lineOff2, options);

					if (options.detectCharDiffs)
					{
						LOGD(LOG_ALGO, "Compare Sections " +
								std::to_string(off1 + 1) + " to " +
								std::to_string(end1 + 1) + " and " +
								std::to_string(off2 + 1) + " to " +
								std::to_string(end2 + 1) + "\n");

						const auto* pSec1 = &sec1;
						const auto* pSec2 = &sec2;

						diffInfo* pBD1 = pBlockDiff1;
						diffInfo* pBD2 = pBlockDiff2;

						// Compare changed words
						auto diffRes = DiffCalc<Char>(sec1, sec2)();
						const std::vector<diff_info<void>> sectionDiffs = std::move(diffRes.first);

						if (diffRes.second)
						{
							std::swap(pSec1, pSec2);
							std::swap(pBD1, pBD2);
							std::swap(off1, off2);
							std::swap(end1, end2);
						}

						PRINT_DIFFS("CHAR DIFFS", sectionDiffs);

						intptr_t matchLen = 0;
						intptr_t matchSections = 0;

						for (const auto& sd: sectionDiffs)
						{
							if (sd.type == diff_type::DIFF_MATCH)
							{
								matchLen += sd.len;
								++matchSections;
							}
						}

						if (matchSections)
						{
							LOGD(LOG_ALGO, "Matching sections found: " + std::to_string(matchSections) +
									", matched len: " + std::to_string(matchLen) + "\n");

							// Are similarities a considerable portion of the diff?
							if ((int)((matchLen * 100) / std::max(sec1.size(), sec2.size())) >=
								options.changedThresholdPercent)
							{
								for (const auto& sd: sectionDiffs)
								{
									if (sd.type == diff_type::DIFF_IN_1)
									{
										section_t change;

										change.off = (*pSec1)[sd.off].pos + off1;
										change.len = (*pSec1)[sd.off + sd.len - 1].pos + off1 + 1 - change.off;

										pBD1->info.changedLines.back().changes.emplace_back(change);
									}
									else if (sd.type == diff_type::DIFF_IN_2)
									{
										section_t change;

										change.off = (*pSec2)[sd.off].pos + off2;
										change.len = (*pSec2)[sd.off + sd.len - 1].pos + off2 + 1 - change.off;

										pBD2->info.changedLines.back().changes.emplace_back(change);
									}
								}

								totalLineMatchLen += matchLen;

								LOGD(LOG_ALGO, "Whole section checked for matches\n");

								++i;
								continue;
							}
							// If not, mark only beginning and ending diff section matches
							else
							{
								const intptr_t matches =
										matchBeginEnd(*pBD1, *pBD2, *pSec1, *pSec2, off1, off2, end1, end2,
												[](const wchar_t) { return true; });

								if (matches)
								{
									totalLineMatchLen += matches;

									++i;
									continue;
								}
							}

							// No matching sections between the lines found - move to next lines
							if (lineDiffsSize == 2)
								break;
						}
					}
					// Always match non-alphabetical characters in the beginning and at the end
					else
					{
						const intptr_t matches =
								matchBeginEnd(*pBlockDiff1, *pBlockDiff2, sec1, sec2, off1, off2, end1, end2,
										[](const wchar_t ch) { return (getCharTypeW(ch) != charType::ALPHANUMCHAR); });

						if (matches)
						{
							totalLineMatchLen += matches;

							++i;
							continue;
						}

						// No matching sections between the lines found - move to next lines
						if (lineDiffsSize == 2)
							break;
					}
				}

				section_t change;

				change.off = (*pLine1)[ld.off].pos;
				change.len = (*pLine1)[ld.off + ld.len - 1].pos + (*pLine1)[ld.off + ld.len - 1].len - change.off;

				pBlockDiff1->info.changedLines.back().changes.emplace_back(change);
			}
		}

		// Not enough portion of the lines matches - consider them totally different
		if (((totalLineMatchLen * 100) / std::max(lineLen1, lineLen2)) < options.changedThresholdPercent)
		{
			pBlockDiff1->info.changedLines.pop_back();
			pBlockDiff2->info.changedLines.pop_back();
		}
	}
}


std::vector<std::set<LinesConv>> getOrderedConvergence(const DocCmpInfo& doc1, const DocCmpInfo& doc2,
		const diffInfo& blockDiff1, const diffInfo& blockDiff2, const CompareOptions& options)
{
	const std::vector<std::vector<Char>> chunk1 = getChars(doc1, blockDiff1, options);
	const std::vector<std::vector<Char>> chunk2 = getChars(doc2, blockDiff2, options);

	const intptr_t linesCount1 = static_cast<intptr_t>(chunk1.size());
	const intptr_t linesCount2 = static_cast<intptr_t>(chunk2.size());

	std::vector<std::set<LinesConv>> lines1Convergence(linesCount1);
	std::vector<std::set<LinesConv>> lines2Convergence(linesCount2);

#if defined(MULTITHREAD) && (MULTITHREAD != 0)
	std::mutex mtx;
#endif

	auto workFn =
		[&](intptr_t startLine, intptr_t endLine)
		{
			progress_ptr& progress = ProgressDlg::Get();

			intptr_t linesProgress = 0;

			for (intptr_t line1 = startLine; line1 < endLine; ++line1)
			{
				if (chunk1[line1].empty())
				{
					linesProgress += linesCount2;
					continue;
				}

				std::vector<Word> words1;

				for (intptr_t line2 = 0; line2 < linesCount2; ++line2)
				{
					if (chunk2[line2].empty())
					{
						++linesProgress;
						continue;
					}

					const intptr_t minSize = std::min(chunk1[line1].size(), chunk2[line2].size());
					const intptr_t maxSize = std::max(chunk1[line1].size(), chunk2[line2].size());

					if (((minSize * 100) / maxSize) < options.changedThresholdPercent)
					{
						++linesProgress;
						continue;
					}

					intptr_t matchesCount	= 0;
					intptr_t diffsCount		= 0;

					auto charDiffs = DiffCalc<Char>(chunk1[line1], chunk2[line2],
							std::bind(&ProgressDlg::IsCancelled, progress))();

					if (progress->IsCancelled())
						return;

					const intptr_t charDiffsSize = static_cast<intptr_t>(charDiffs.first.size());

					for (intptr_t i = 0; i < charDiffsSize; ++i)
					{
						if (charDiffs.first[i].type == diff_type::DIFF_MATCH)
						{
							matchesCount += charDiffs.first[i].len;
						}
						else if (options.bestSeqChangedLines)
						{
							++diffsCount;

							// Count replacement as a single diff
							if ((i + 1 < charDiffsSize) && (charDiffs.first[i + 1].type == diff_type::DIFF_IN_2))
								++i;
						}
					}

					if (((matchesCount * 100) / maxSize) >= options.changedThresholdPercent)
					{
						const float lineConvergence = (static_cast<float>(matchesCount) * 100) / maxSize;

						Conv conv(lineConvergence, diffsCount);

#if defined(MULTITHREAD) && (MULTITHREAD != 0)
						std::lock_guard<std::mutex> lock(mtx);
#endif

						if (!progress->Advance(linesProgress + 1))
							return;

						linesProgress = 0;

						bool addL1C = false;

						if (lines2Convergence[line2].empty() || (conv == lines2Convergence[line2].begin()->conv))
						{
							lines2Convergence[line2].emplace(conv, line1, line2);
							addL1C = true;
						}
						else if (conv > lines2Convergence[line2].begin()->conv)
						{
							for (const auto& l2c : lines2Convergence[line2])
							{
								auto& l1c = lines1Convergence[l2c.line1];

								if (!l1c.empty())
								{
									for (auto l1cI = l1c.begin(); l1cI != l1c.end(); ++l1cI)
									{
										if (l1cI->line2 == line2)
										{
											l1c.erase(l1cI);
											break;
										}
									}
								}
							}

							lines2Convergence[line2].clear();
							lines2Convergence[line2].emplace(conv, line1, line2);
							addL1C = true;
						}

						if (addL1C)
						{
							if (lines1Convergence[line1].empty() || (conv == lines1Convergence[line1].begin()->conv))
							{
								lines1Convergence[line1].emplace(conv, line1, line2);
							}
							else if (conv > lines1Convergence[line1].begin()->conv)
							{
								lines1Convergence[line1].clear();
								lines1Convergence[line1].emplace(conv, line1, line2);
							}
						}
					}
					else
					{
#if defined(MULTITHREAD) && (MULTITHREAD != 0)
						std::lock_guard<std::mutex> lock(mtx);
#endif

						if (!progress->Advance(linesProgress + 1))
							return;

						linesProgress = 0;
					}
				}
			}
		};

#if defined(MULTITHREAD) && (MULTITHREAD != 0)

	auto threadFn =
		[&workFn](intptr_t startLine, intptr_t endLine)
		{
			try
			{
				workFn(startLine, endLine);
			}
			catch (std::exception& e)
			{
				char msg[128];
				_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
				::MessageBoxA(nppData._nppHandle, msg, "ComparePlus", MB_OK | MB_ICONWARNING);
			}
			catch (...)
			{
				::MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "ComparePlus",
						MB_OK | MB_ICONWARNING);
			}
		};

	int threadsCount = std::thread::hardware_concurrency() - 1;

	if (threadsCount < 1)
		threadsCount = 1;
	else if (static_cast<intptr_t>(threadsCount) > linesCount1)
		threadsCount = static_cast<int>(linesCount1);

	if (threadsCount > 1)
	{
		constexpr intptr_t jobsPerThread = 50;

		const intptr_t totalJobs		= linesCount1 * linesCount2;
		const intptr_t threadsNeeded	= (totalJobs + jobsPerThread - 1) / jobsPerThread;

		if (static_cast<intptr_t>(threadsCount) > threadsNeeded)
			threadsCount = static_cast<int>(threadsNeeded);
	}

	if (threadsCount > 1)
	{
		const intptr_t linesPerThread = (linesCount1 + threadsCount - 1) / threadsCount;

		LOGD(LOG_ALGO, "getOrderedConvergence(): threads to use: " + std::to_string(threadsCount) +
				", jobs per thread: " + std::to_string(linesPerThread * linesCount2) + "\n");

		std::vector<std::thread> threads;

		for (intptr_t startLine1 = 0; startLine1 < linesCount1; startLine1 += linesPerThread)
		{
			const intptr_t endLine1 = std::min(startLine1 + linesPerThread, linesCount1);

			LOGD(LOG_ALGO, "Thread line1 range: " + std::to_string(startLine1 + 1) +
					" to " + std::to_string(endLine1) + "\n");

			try
			{
				threads.emplace_back(threadFn, startLine1, endLine1);
			}
			catch (...)
			{
				workFn(startLine1, linesCount1);
				break;
			}
		}

		for (auto& th : threads)
			th.join();
	}
	else
	{
		LOGD(LOG_ALGO, "getOrderedConvergence(): using 1 thread\n");

		workFn(0, linesCount1);
	}

#else

	workFn(0, linesCount1);

#endif // MULTITHREAD

	return lines1Convergence;
}


bool compareBlocks(const DocCmpInfo& doc1, const DocCmpInfo& doc2, diffInfo& blockDiff1, diffInfo& blockDiff2,
		const CompareOptions& options)
{
	std::vector<std::set<LinesConv>> orderedLinesConvergence =
			getOrderedConvergence(doc1, doc2, blockDiff1, blockDiff2, options);

	{
		progress_ptr& progress = ProgressDlg::Get();

		if (progress->IsCancelled())
			return false;
	}

#ifdef DLOG
	for (const auto& oc: orderedLinesConvergence)
	{
		if (!oc.empty())
			LOGD(LOG_ALGO, "Best Matching Lines: " +
					std::to_string(doc1.lines[oc.begin()->line1 + blockDiff1.off].line + 1) + " and " +
					std::to_string(doc2.lines[oc.begin()->line2 + blockDiff2.off].line + 1) + "\n");
	}
#endif

	std::map<intptr_t, intptr_t> bestLineMappings; // line2 -> line1
	{
		std::vector<std::map<intptr_t, intptr_t>> groupedLines;

		for (const auto& oc: orderedLinesConvergence)
		{
			if (oc.empty())
				continue;

			if (groupedLines.empty())
			{
				auto ocItr = oc.begin();

				groupedLines.emplace_back();
				groupedLines.back().emplace(ocItr->line2, ocItr->line1);

				continue;
			}

			intptr_t addToIdx = -1;

			for (auto ocItr = oc.begin(); ocItr != oc.end(); ++ocItr)
			{
				for (intptr_t i = 0; i < static_cast<intptr_t>(groupedLines.size()); ++i)
				{
					const auto& gl = groupedLines[i];

					if ((ocItr->line2) > (gl.rbegin()->first))
					{
						if (addToIdx == -1)
						{
							addToIdx = i;
						}
						else if (groupedLines[addToIdx].size() < gl.size())
						{
							groupedLines.erase(groupedLines.begin() + addToIdx);
							addToIdx = --i;
						}
						else
						{
							groupedLines.erase(groupedLines.begin() + i);
							--i;
						}
					}
				}

				if (addToIdx != -1)
				{
					auto& gl = groupedLines[addToIdx];
					gl.emplace_hint(gl.end(), ocItr->line2, ocItr->line1);

					break;
				}
			}

			if (addToIdx != -1)
				continue;

			auto ocrItr = oc.rbegin();

			std::map<intptr_t, intptr_t> subGroup;

			for (intptr_t i = 0; i < static_cast<intptr_t>(groupedLines.size()); ++i)
			{
				auto& gl = groupedLines[i];

				auto glResItr = gl.emplace(ocrItr->line2, ocrItr->line1);

				if (glResItr.second)
				{
					std::map<intptr_t, intptr_t> newSubGroup;
					auto sgEndItr = glResItr.first;

					newSubGroup.insert(gl.begin(), ++sgEndItr);
					gl.erase(glResItr.first);

					if (newSubGroup.size() > subGroup.size())
						subGroup = std::move(newSubGroup);
				}
			}

			if (!subGroup.empty())
			{
				groupedLines.emplace_back(std::move(subGroup));

				LOGD(LOG_ALGO, "New lines group (" + std::to_string(groupedLines.size()) + " total). Last lines: " +
						std::to_string(doc1.lines[ocrItr->line1 + blockDiff1.off].line + 1) + " - " +
						std::to_string(doc2.lines[ocrItr->line2 + blockDiff2.off].line + 1) + "\n");
			}
		}

		if (groupedLines.empty())
			return true;

		intptr_t	bestGroupIdx = 0;
		size_t		bestSize = groupedLines[0].size();

		for (intptr_t i = 1; i < static_cast<intptr_t>(groupedLines.size()); ++i)
		{
			if (bestSize < groupedLines[i].size())
			{
				bestSize = groupedLines[i].size();
				bestGroupIdx = i;
			}
		}

		bestLineMappings = std::move(groupedLines[bestGroupIdx]);
	}

	compareLines(doc1, doc2, blockDiff1, blockDiff2, bestLineMappings, options);

	return true;
}


void markSection(const DocCmpInfo& doc, const diffInfo& bd, const CompareOptions& options)
{
	const intptr_t endOff = doc.section.off + doc.section.len;

	for (intptr_t i = doc.section.off, line = bd.off + doc.section.off; i < endOff; ++i, ++line)
	{
		intptr_t movedLen = bd.info.movedSection(i);

		if (movedLen > doc.section.len)
			movedLen = doc.section.len;

		if (movedLen == 0)
		{
			intptr_t prevLine = doc.lines[line].line + 1;

			for (; (i < endOff) && (bd.info.movedSection(i) == 0); ++i, ++line)
			{
				const intptr_t docLine = doc.lines[line].line;
				const int mark = (doc.nonUniqueLines.find(docLine) == doc.nonUniqueLines.end()) ? doc.blockDiffMask :
						(doc.blockDiffMask == MARKER_MASK_ADDED) ? MARKER_MASK_ADDED_LOCAL : MARKER_MASK_REMOVED_LOCAL;

				CallScintilla(doc.view, SCI_MARKERADDSET, docLine, mark);

				if (options.ignoreEmptyLines && !options.neverMarkIgnored)
				{
					for (; prevLine < docLine; ++prevLine)
						CallScintilla(doc.view, SCI_MARKERADDSET, prevLine, doc.blockDiffMask & MARKER_MASK_LINE);

					prevLine = docLine + 1;
				}
			}

			--i;
			--line;
		}
		else if (movedLen == 1)
		{
			CallScintilla(doc.view, SCI_MARKERADDSET, doc.lines[line].line, MARKER_MASK_MOVED_LINE);
		}
		else
		{
			CallScintilla(doc.view, SCI_MARKERADDSET, doc.lines[line].line, MARKER_MASK_MOVED_BEGIN);

			i += --movedLen;

			intptr_t prevLine = doc.lines[line].line + 1;
			intptr_t endLine = line + movedLen;

			for (++line; line < endLine; ++line)
			{
				const intptr_t docLine = doc.lines[line].line;
				CallScintilla(doc.view, SCI_MARKERADDSET, docLine, MARKER_MASK_MOVED_MID);

				if (options.ignoreEmptyLines && !options.neverMarkIgnored)
				{
					for (; prevLine < docLine; ++prevLine)
						CallScintilla(doc.view, SCI_MARKERADDSET, prevLine, MARKER_MASK_MOVED_MID & MARKER_MASK_LINE);

					prevLine = docLine + 1;
				}
			}

			const intptr_t docLine = doc.lines[line].line;
			CallScintilla(doc.view, SCI_MARKERADDSET, docLine, MARKER_MASK_MOVED_END);

			if (options.ignoreEmptyLines && !options.neverMarkIgnored)
			{
				for (; prevLine < docLine; ++prevLine)
					CallScintilla(doc.view, SCI_MARKERADDSET, prevLine, MARKER_MASK_MOVED_MID & MARKER_MASK_LINE);
			}
		}
	}
}


void markLineDiffs(const CompareInfo& cmpInfo, const diffInfo& bd, intptr_t lineIdx)
{
	intptr_t line = cmpInfo.doc1.lines[bd.off + bd.info.changedLines[lineIdx].line].line;
	intptr_t linePos = getLineStart(cmpInfo.doc1.view, line);
	int color = (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED) ?
			Settings.colors().add_highlight : Settings.colors().rem_highlight;

	for (const auto& change: bd.info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc1.view, linePos + change.off, change.len, color);

	CallScintilla(cmpInfo.doc1.view, SCI_MARKERADDSET, line,
			cmpInfo.doc1.nonUniqueLines.find(line) == cmpInfo.doc1.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);

	line = cmpInfo.doc2.lines[bd.info.matchBlock->off + bd.info.matchBlock->info.changedLines[lineIdx].line].line;
	linePos = getLineStart(cmpInfo.doc2.view, line);
	color = (cmpInfo.doc2.blockDiffMask == MARKER_MASK_ADDED) ?
			Settings.colors().add_highlight : Settings.colors().rem_highlight;

	for (const auto& change: bd.info.matchBlock->info.changedLines[lineIdx].changes)
		markTextAsChanged(cmpInfo.doc2.view, linePos + change.off, change.len, color);

	CallScintilla(cmpInfo.doc2.view, SCI_MARKERADDSET, line,
			cmpInfo.doc2.nonUniqueLines.find(line) == cmpInfo.doc2.nonUniqueLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);
}


bool markAllDiffs(CompareInfo& cmpInfo, const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.clear();

	const intptr_t blockDiffSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	progress->SetMaxCount(blockDiffSize);

	std::pair<intptr_t, intptr_t> alignLines {0, 0};

	AlignmentPair alignPair;

	AlignmentViewData* pMainAlignData	= &alignPair.main;
	AlignmentViewData* pSubAlignData	= &alignPair.sub;

	// Make sure pMainAlignData is linked to doc1
	if (cmpInfo.doc1.view == SUB_VIEW)
		std::swap(pMainAlignData, pSubAlignData);

	for (intptr_t i = 0; i < blockDiffSize; ++i)
	{
		const diffInfo& bd = cmpInfo.blockDiffs[i];

		if (bd.type == diff_type::DIFF_MATCH)
		{
			pMainAlignData->diffMask	= 0;
			pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

			pSubAlignData->diffMask		= 0;
			pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

			summary.alignmentInfo.emplace_back(alignPair);

			if (options.alignAllMatches)
			{
				// Align all pairs of matching lines
				for (intptr_t j = bd.len - 1; j; --j)
				{
					++alignLines.first;
					++alignLines.second;

					pMainAlignData->line	= cmpInfo.doc1.lines[alignLines.first].line;
					pSubAlignData->line		= cmpInfo.doc2.lines[alignLines.second].line;

					summary.alignmentInfo.emplace_back(alignPair);
				}

				++alignLines.first;
				++alignLines.second;
			}
			else
			{
				if (options.ignoreEmptyLines)
				{
					// Align pairs of matching lines after ignored lines sections
					for (intptr_t j = bd.len - 1; j; --j)
					{
						++alignLines.first;
						++alignLines.second;

						if ((++pMainAlignData->line != cmpInfo.doc1.lines[alignLines.first].line) ||
							(++pSubAlignData->line != cmpInfo.doc2.lines[alignLines.second].line))
						{
							pMainAlignData->line	= cmpInfo.doc1.lines[alignLines.first].line;
							pSubAlignData->line		= cmpInfo.doc2.lines[alignLines.second].line;

							summary.alignmentInfo.emplace_back(alignPair);
						}
					}

					++alignLines.first;
					++alignLines.second;
				}
				else
				{
					alignLines.first	+= bd.len;
					alignLines.second	+= bd.len;
				}
			}

			summary.match += bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_2)
		{
			cmpInfo.doc2.section.off = 0;
			cmpInfo.doc2.section.len = bd.len;
			markSection(cmpInfo.doc2, bd, options);

			pMainAlignData->diffMask	= 0;
			pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

			pSubAlignData->diffMask		= cmpInfo.doc2.blockDiffMask;
			pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

			summary.alignmentInfo.emplace_back(alignPair);

			const intptr_t movedLines = bd.info.movedCount();

			summary.diffLines	+= bd.len;
			summary.moved		+= movedLines;

			if (cmpInfo.doc2.blockDiffMask == MARKER_MASK_ADDED)
				summary.added += bd.len - movedLines;
			else
				summary.removed += bd.len - movedLines;

			alignLines.second += bd.len;
		}
		else if (bd.type == diff_type::DIFF_IN_1)
		{
			if (bd.info.matchBlock)
			{
				const intptr_t changedLinesCount = static_cast<intptr_t>(bd.info.changedLines.size());

				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc2.section.off = 0;

				for (intptr_t j = 0; j < changedLinesCount; ++j)
				{
					cmpInfo.doc1.section.len = bd.info.changedLines[j].line - cmpInfo.doc1.section.off;
					cmpInfo.doc2.section.len = bd.info.matchBlock->info.changedLines[j].line - cmpInfo.doc2.section.off;

					if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
					{
						pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
						pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

						pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;
						pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

						summary.alignmentInfo.emplace_back(alignPair);

						if (options.ignoreEmptyLines && options.neverMarkIgnored &&
							cmpInfo.doc1.section.len && cmpInfo.doc2.section.len)
						{
							std::vector<intptr_t> alignLines1;
							intptr_t maxLines = cmpInfo.doc1.section.len + alignLines.first;

							for (intptr_t l = alignLines.first + 1; l < maxLines; ++l)
							{
								if (cmpInfo.doc1.lines[l].line - cmpInfo.doc1.lines[l - 1].line > 1)
									alignLines1.emplace_back(l);
							}

							if (!alignLines1.empty())
							{
								std::vector<intptr_t> alignLines2;
								maxLines = cmpInfo.doc2.section.len + alignLines.second;

								for (intptr_t l = alignLines.second + 1; l < maxLines; ++l)
								{
									if (cmpInfo.doc2.lines[l].line - cmpInfo.doc2.lines[l - 1].line > 1)
										alignLines2.emplace_back(l);
								}

								maxLines = std::min(alignLines1.size(), alignLines2.size());

								for (intptr_t l = 0; l < maxLines; ++l)
								{
									pMainAlignData->line	= toAlignmentLine(cmpInfo.doc1, alignLines1[l]);
									pSubAlignData->line		= toAlignmentLine(cmpInfo.doc2, alignLines2[l]);

									summary.alignmentInfo.emplace_back(alignPair);
								}
							}
						}

						if (cmpInfo.doc1.section.len)
						{
							markSection(cmpInfo.doc1, bd, options);
							alignLines.first += cmpInfo.doc1.section.len;
						}

						if (cmpInfo.doc2.section.len)
						{
							markSection(cmpInfo.doc2, *bd.info.matchBlock, options);
							alignLines.second += cmpInfo.doc2.section.len;
						}

						summary.diffLines += std::max(cmpInfo.doc1.section.len, cmpInfo.doc2.section.len);
					}

					pMainAlignData->diffMask	= MARKER_MASK_CHANGED;
					pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

					pSubAlignData->diffMask		= MARKER_MASK_CHANGED;
					pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

					summary.alignmentInfo.emplace_back(alignPair);

					markLineDiffs(cmpInfo, bd, j);

					cmpInfo.doc1.section.off = bd.info.changedLines[j].line + 1;
					cmpInfo.doc2.section.off = bd.info.matchBlock->info.changedLines[j].line + 1;

					++alignLines.first;
					++alignLines.second;
				}

				cmpInfo.doc1.section.len = bd.len - cmpInfo.doc1.section.off;
				cmpInfo.doc2.section.len = bd.info.matchBlock->len - cmpInfo.doc2.section.off;

				if (cmpInfo.doc1.section.len || cmpInfo.doc2.section.len)
				{
					pMainAlignData->diffMask	= cmpInfo.doc1.section.len ? cmpInfo.doc1.blockDiffMask : 0;
					pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

					pSubAlignData->diffMask		= cmpInfo.doc2.section.len ? cmpInfo.doc2.blockDiffMask : 0;
					pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

					summary.alignmentInfo.emplace_back(alignPair);

					if (options.ignoreEmptyLines && options.neverMarkIgnored &&
						cmpInfo.doc1.section.len && cmpInfo.doc2.section.len)
					{
						std::vector<intptr_t> alignLines1;
						intptr_t maxLines = cmpInfo.doc1.section.len + alignLines.first;

						for (intptr_t l = alignLines.first + 1; l < maxLines; ++l)
						{
							if (cmpInfo.doc1.lines[l].line - cmpInfo.doc1.lines[l - 1].line > 1)
								alignLines1.emplace_back(l);
						}

						if (!alignLines1.empty())
						{
							std::vector<intptr_t> alignLines2;
							maxLines = cmpInfo.doc2.section.len + alignLines.second;

							for (intptr_t l = alignLines.second + 1; l < maxLines; ++l)
							{
								if (cmpInfo.doc2.lines[l].line - cmpInfo.doc2.lines[l - 1].line > 1)
									alignLines2.emplace_back(l);
							}

							maxLines = std::min(alignLines1.size(), alignLines2.size());

							for (intptr_t l = 0; l < maxLines; ++l)
							{
								pMainAlignData->line	= toAlignmentLine(cmpInfo.doc1, alignLines1[l]);
								pSubAlignData->line		= toAlignmentLine(cmpInfo.doc2, alignLines2[l]);

								summary.alignmentInfo.emplace_back(alignPair);
							}
						}
					}

					if (cmpInfo.doc1.section.len)
					{
						markSection(cmpInfo.doc1, bd, options);
						alignLines.first += cmpInfo.doc1.section.len;
					}

					if (cmpInfo.doc2.section.len)
					{
						markSection(cmpInfo.doc2, *bd.info.matchBlock, options);
						alignLines.second += cmpInfo.doc2.section.len;
					}

					summary.diffLines += std::max(cmpInfo.doc1.section.len, cmpInfo.doc2.section.len);
				}

				const intptr_t movedLines1 = bd.info.movedCount();
				const intptr_t movedLines2 = bd.info.matchBlock->info.movedCount();

				const intptr_t newLines1 = bd.len - changedLinesCount - movedLines1;
				const intptr_t newLines2 = bd.info.matchBlock->len - changedLinesCount - movedLines2;

				summary.diffLines	+= changedLinesCount;
				summary.changed		+= changedLinesCount;
				summary.moved		+= movedLines1 + movedLines2;

				if (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED)
				{
					summary.added	+= newLines1;
					summary.removed	+= newLines2;
				}
				else
				{
					summary.added	+= newLines2;
					summary.removed	+= newLines1;
				}

				++i;
			}
			else
			{
				cmpInfo.doc1.section.off = 0;
				cmpInfo.doc1.section.len = bd.len;
				markSection(cmpInfo.doc1, bd, options);

				pMainAlignData->diffMask	= cmpInfo.doc1.blockDiffMask;
				pMainAlignData->line		= toAlignmentLine(cmpInfo.doc1, alignLines.first);

				pSubAlignData->diffMask		= 0;
				pSubAlignData->line			= toAlignmentLine(cmpInfo.doc2, alignLines.second);

				summary.alignmentInfo.emplace_back(alignPair);

				const intptr_t movedLines = bd.info.movedCount();

				summary.diffLines	+= bd.len;
				summary.moved		+= movedLines;

				if (cmpInfo.doc1.blockDiffMask == MARKER_MASK_ADDED)
					summary.added += bd.len - movedLines;
				else
					summary.removed += bd.len - movedLines;

				alignLines.first += bd.len;
			}
		}

		if (!progress->Advance())
			return false;
	}

	summary.moved /= 2;

	if (!progress->NextPhase())
		return false;

	return true;
}


CompareResult runCompare(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	CompareInfo cmpInfo;

	cmpInfo.doc1.view	= MAIN_VIEW;
	cmpInfo.doc2.view	= SUB_VIEW;

	if (options.selectionCompare)
	{
		cmpInfo.doc1.section.off	= options.selections[MAIN_VIEW].first;
		cmpInfo.doc1.section.len	= options.selections[MAIN_VIEW].second - options.selections[MAIN_VIEW].first + 1;

		cmpInfo.doc2.section.off	= options.selections[SUB_VIEW].first;
		cmpInfo.doc2.section.len	= options.selections[SUB_VIEW].second - options.selections[SUB_VIEW].first + 1;
	}

	cmpInfo.doc1.blockDiffMask = (options.newFileViewId == MAIN_VIEW) ? MARKER_MASK_ADDED : MARKER_MASK_REMOVED;
	cmpInfo.doc2.blockDiffMask = (options.newFileViewId == MAIN_VIEW) ? MARKER_MASK_REMOVED : MARKER_MASK_ADDED;

	LOGD_GET_TIME;

	getLines(cmpInfo.doc1, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(cmpInfo.doc2, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	auto diffRes = DiffCalc<Line, blockDiffInfo>(cmpInfo.doc1.lines, cmpInfo.doc2.lines,
			std::bind(&ProgressDlg::IsCancelled, progress))(true, true);

	if (progress->IsCancelled())
		return CompareResult::COMPARE_CANCELLED;

	cmpInfo.blockDiffs = std::move(diffRes.first);

	if (diffRes.second)
		swap(cmpInfo.doc1, cmpInfo.doc2);

	LOGD_GET_TIME;
	PRINT_DIFFS("COMPARE START - LINE DIFFS", cmpInfo.blockDiffs);

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	if (blockDiffsSize == 0 || (blockDiffsSize == 1 && cmpInfo.blockDiffs[0].type == diff_type::DIFF_MATCH))
		return CompareResult::COMPARE_MATCH;

	findUniqueLines(cmpInfo);

	if (options.detectMoves)
		findMoves(cmpInfo);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::vector<intptr_t> changedBlockIdx;

	intptr_t changedProgressCount = 0;

	// Get changed blocks to sub-compare
	for (intptr_t i = 1; i < blockDiffsSize; ++i)
	{
		if ((cmpInfo.blockDiffs[i].type == diff_type::DIFF_IN_2) &&
				(cmpInfo.blockDiffs[i - 1].type == diff_type::DIFF_IN_1))
		{
			changedProgressCount += cmpInfo.blockDiffs[i].len * cmpInfo.blockDiffs[i - 1].len;
			changedBlockIdx.emplace_back(i++);
		}
	}

	progress->SetMaxCount(changedProgressCount);

	if (changedProgressCount > 10000)
		progress->Show();

	// Do block compares
	for (intptr_t i: changedBlockIdx)
	{
		diffInfo& blockDiff1 = cmpInfo.blockDiffs[i - 1];
		diffInfo& blockDiff2 = cmpInfo.blockDiffs[i];

		blockDiff1.info.matchBlock = &blockDiff2;
		blockDiff2.info.matchBlock = &blockDiff1;

		if (!compareBlocks(cmpInfo.doc1, cmpInfo.doc2, blockDiff1, blockDiff2, options))
			return CompareResult::COMPARE_CANCELLED;
	}

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	clearWindow(MAIN_VIEW);
	clearWindow(SUB_VIEW);

	if (!markAllDiffs(cmpInfo, options, summary))
		return CompareResult::COMPARE_CANCELLED;

	return CompareResult::COMPARE_MISMATCH;
}


CompareResult runFindUnique(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.alignmentInfo.clear();

	summary.diffLines	= 0;
	summary.added		= 0;
	summary.removed		= 0;
	summary.moved		= 0;
	summary.changed		= 0;
	summary.match		= 0;

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

	if (options.newFileViewId == MAIN_VIEW)
	{
		doc1.blockDiffMask = MARKER_MASK_ADDED;
		doc2.blockDiffMask = MARKER_MASK_REMOVED;
	}
	else
	{
		doc1.blockDiffMask = MARKER_MASK_REMOVED;
		doc2.blockDiffMask = MARKER_MASK_ADDED;
	}

	getLines(doc1, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(doc2, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<intptr_t>> doc1UniqueLines;

	for (const auto& line: doc1.lines)
	{
		auto insertPair = doc1UniqueLines.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	doc1.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<uint64_t, std::vector<intptr_t>> doc2UniqueLines;

	for (const auto& line: doc2.lines)
	{
		auto insertPair = doc2UniqueLines.emplace(line.hash, std::vector<intptr_t>{line.line});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.line);
	}

	doc2.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	clearWindow(MAIN_VIEW);
	clearWindow(SUB_VIEW);

	intptr_t doc1UniqueLinesCount = 0;

	for (const auto& uniqueLine: doc1UniqueLines)
	{
		auto doc2it = doc2UniqueLines.find(uniqueLine.first);

		if (doc2it != doc2UniqueLines.end())
		{
			doc2UniqueLines.erase(doc2it);
			++summary.match;
		}
		else
		{
			for (const auto& line: uniqueLine.second)
			{
				CallScintilla(doc1.view, SCI_MARKERADDSET, line, doc1.blockDiffMask);
				++doc1UniqueLinesCount;
			}
		}
	}

	if (doc1UniqueLinesCount == 0 && doc2UniqueLines.empty())
		return CompareResult::COMPARE_MATCH;

	if (doc1.blockDiffMask == MARKER_MASK_ADDED)
		summary.added = doc1UniqueLinesCount;
	else
		summary.removed = doc1UniqueLinesCount;

	for (const auto& uniqueLine: doc2UniqueLines)
	{
		for (const auto& line: uniqueLine.second)
			CallScintilla(doc2.view, SCI_MARKERADDSET, line, doc2.blockDiffMask);

		if (doc2.blockDiffMask == MARKER_MASK_ADDED)
			summary.added += uniqueLine.second.size();
		else
			summary.removed += uniqueLine.second.size();
	}

	AlignmentPair align;
	align.main.line	= doc1.section.off;
	align.sub.line	= doc2.section.off;

	summary.alignmentInfo.push_back(align);

	return CompareResult::COMPARE_MISMATCH;
}

}


CompareResult compareViews(const CompareOptions& options, const TCHAR* progressInfo, CompareSummary& summary)
{
	CompareResult result = CompareResult::COMPARE_ERROR;

	if (!progressInfo || !ProgressDlg::Open(progressInfo))
		return CompareResult::COMPARE_ERROR;

	try
	{
		if (options.findUniqueMode)
			result = runFindUnique(options, summary);
		else
			result = runCompare(options, summary);

		ProgressDlg::Close();

		if (result != CompareResult::COMPARE_MISMATCH)
		{
			clearWindow(MAIN_VIEW);
			clearWindow(SUB_VIEW);
		}
	}
	catch (std::exception& e)
	{
		ProgressDlg::Close();

		clearWindow(MAIN_VIEW);
		clearWindow(SUB_VIEW);

		char msg[128];
		_snprintf_s(msg, _countof(msg), _TRUNCATE, "Exception occurred: %s", e.what());
		::MessageBoxA(nppData._nppHandle, msg, "ComparePlus", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		ProgressDlg::Close();

		::MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "ComparePlus", MB_OK | MB_ICONWARNING);
	}

	return result;
}
