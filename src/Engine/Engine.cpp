/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2026 Pavel Nedev (pg.nedev@gmail.com)
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

#define NOMINMAX	1

#include <cassert>
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

#include "Tools.h"
#include "Engine.h"
#include "diff.h"
#include "ProgressDlg.h"


#ifdef MULTITHREAD

#include <atomic>

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#else // MULTITHREAD not defined

#pragma message("Multithread change detection disabled.")

#endif // MULTITHREAD


namespace {

static constexpr uint64_t cHashSeed = 0x84222325;


enum class charType
{
	SPACECHAR,
	ALPHANUMCHAR,
	OTHERCHAR
};


// Compared element 'Line'
struct Line : public hash_type<uint64_t>
{
	Line(intptr_t l = 0, uint64_t h = cHashSeed) : hash_type<uint64_t>(h), num(l) {}

	intptr_t num;
};


// Compared element 'Word'
struct Word : public hash_type<uint64_t>
{
	Word(intptr_t idx, intptr_t p, intptr_t l, uint64_t h = cHashSeed) :
		hash_type<uint64_t>(h), lineIdx(idx),  pos(p), len(l) {}

	intptr_t lineIdx;
	intptr_t pos;
	intptr_t len;
};


// Compared element 'Char'
// Use directly the character as hash
struct Char : public hash_type<wchar_t>
{
	Char(wchar_t c, intptr_t p) : hash_type<wchar_t>(c), pos(p) {}

	intptr_t pos;
};


struct changed_range_t : public range_t
{
	changed_range_t() : range_t(), moved {false} {}
	changed_range_t(intptr_t start, intptr_t end) : range_t(start, end), moved {false} {}

	bool moved;
};


struct ChangedLine
{
	ChangedLine(intptr_t l) : idx(l) {}

	intptr_t idx;
	std::vector<changed_range_t> changes;
};


struct MovedRanges : public std::vector<range_t>
{
	inline intptr_t totalLinesCount() const
	{
		intptr_t count = 0;

		for (const auto& r : *this)
			count += r.len();

		return count;
	}

	inline intptr_t rangeLen(intptr_t idx) const
	{
		for (const auto& r : *this)
		{
			if (r.contains(idx))
				return r.len();
		}

		return 0;
	}

	inline bool contain(intptr_t idx) const
	{
		return (rangeLen(idx) != 0);
	}

	inline bool getNextUnmoved(intptr_t& idx) const
	{
		for (const auto& r : *this)
		{
			if (r.contains(idx))
			{
				idx = r.e;
				return true;
			}
		}

		return false;
	}
};


struct DocCmpInfo
{
	DocCmpInfo(int v, const range_t	diff_info::*dptr) : view(v), diffPtr(dptr) {};
	DocCmpInfo(const DocCmpInfo&) = delete;
	DocCmpInfo(DocCmpInfo&&) = delete;
	DocCmpInfo& operator=(const DocCmpInfo&) = delete;

	const int	view;
	range_t		range;
	int			diffMask;

	const range_t	diff_info::*diffPtr;

	std::vector<Line>				lines; // Compared lines from 'range' member (vector's index is not a doc line!)
	std::unordered_set<intptr_t>	nonUniqueDocLines;

	std::vector<std::vector<ChangedLine>>	changedLines;	// Changed lines per block diff after lines compare
	std::vector<MovedRanges>				movedRanges;	// Moved lines ranges per block diff after lines compare

	inline const range_t& diffRange(const diff_info& di) const
	{
		return (di.*diffPtr);
	}

	inline const Line& getLine(intptr_t idx) const
	{
		assert(idx >= 0 && idx < (intptr_t)lines.size());

		return lines[idx];
	}

	inline const Line& getLine(const diff_info& di, intptr_t off = 0) const
	{
		assert(off >= 0 && off < (intptr_t)diffRange(di).len());

		return lines[diffRange(di).s + off];
	}

	inline intptr_t getDocLine(intptr_t idx) const
	{
		return getLine(idx).num;
	}

	inline intptr_t getDocLine(const diff_info& di, intptr_t off = 0) const
	{
		return getLine(di, off).num;
	}
};


struct CompareInfo
{
	CompareInfo() : a(MAIN_VIEW, &diff_info::a), b(SUB_VIEW, &diff_info::b) {};
	CompareInfo(const CompareInfo&) = delete;
	CompareInfo(CompareInfo&&) = delete;
	CompareInfo& operator=(const CompareInfo&) = delete;

	DocCmpInfo a;
	DocCmpInfo b;

	diff_results blockDiffs;
};


struct LinesConv
{
	float conv;

	intptr_t lineA;
	intptr_t lineB;

	LinesConv() : lineA(-1), lineB(-1) {}
	LinesConv(float c, intptr_t la, intptr_t lb) : conv(c), lineA(la), lineB(lb) {}

	inline void Set(float c, intptr_t la, intptr_t lb)
	{
		conv = c;
		lineA = la;
		lineB = lb;
	}

	inline bool operator<(const LinesConv& rhs) const
	{
		return ((conv > rhs.conv) ||
				((conv == rhs.conv) && ((lineB < rhs.lineB) ||
										((lineB == rhs.lineB) && (lineA < rhs.lineA)))));
	}
};

template<typename CharT>
inline uint64_t Hash(uint64_t hval, CharT letter)
{
	hval ^= static_cast<uint64_t>(letter);

	hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);

	return hval;
}


inline uint64_t getSectionRangeHash(uint64_t hashSeed, std::vector<wchar_t>& sec, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos >= endPos)
		return hashSeed;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = sec[endPos];

		sec[endPos] = L'\0';

		::CharLowerW((LPWSTR)sec.data() + pos);

		sec[endPos] = storedChar;
	}

	for (; pos < endPos; ++pos)
	{
		if (options.ignoreAllSpaces && (sec[pos] == L' ' || sec[pos] == L'\t'))
			continue;

		if (options.ignoreChangedSpaces && (sec[pos] == L' ' || sec[pos] == L'\t'))
		{
			hashSeed = Hash(hashSeed, L' ');

			while (++pos < endPos && (sec[pos] == L' ' || sec[pos] == L'\t'));

			if (pos == endPos)
				return hashSeed;
		}

		hashSeed = Hash(hashSeed, sec[pos]);
	}

	return hashSeed;
}


uint64_t getRegexIgnoreLineHash(int view, intptr_t off, uint64_t hashSeed, int codepage, const std::vector<char>& line,
	const CompareOptions& options)
{
	const int len = static_cast<int>(line.size());

	if (len == 0)
		return hashSeed;

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

#ifndef MULTITHREAD
	LOGD(LOG_ALGO, "line len " + std::to_string(len) + " to wide char len " + std::to_string(wLen) + "\n");
#endif

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(wLine.begin(), wLine.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	intptr_t mbPos = 0;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		intptr_t pos = 0;

		for (; rit != rend; ++rit)
		{
#ifndef MULTITHREAD
			LOGD(LOG_ALGO, "pos " + std::to_string(rit->position()) + ", len " + std::to_string(rit->length()) + "\n");
#endif
			hashSeed = getSectionRangeHash(hashSeed, wLine, rit->position(), rit->position() + rit->length(), options);

			if (options.highlightRegexIgnores)
			{
				const int mbLen = ::WideCharToMultiByte(codepage, 0,
						wLine.data() + pos, static_cast<int>(rit->position() - pos), NULL, 0, NULL, NULL);

				markTextAsChanged(view, off + mbPos, mbLen, Settings.colors().blank);

				pos = rit->position() + rit->length();
				mbPos += mbLen + ::WideCharToMultiByte(codepage, 0,
						wLine.data() + rit->position(), static_cast<int>(rit->length()), NULL, 0, NULL, NULL);
			}
		}

		if (options.highlightRegexIgnores)
			markTextAsChanged(view, off + mbPos, len - 1 - mbPos, Settings.colors().blank);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t highlightPos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
		{
			while (++pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'));

			if (pos == endPos)
				return hashSeed;
		}

		while (rit != rend)
		{
#ifndef MULTITHREAD
			LOGD(LOG_ALGO, "pos " + std::to_string(rit->position()) + ", len " + std::to_string(rit->length()) + "\n");
#endif
			hashSeed = getSectionRangeHash(hashSeed, wLine, pos, rit->position(), options);

			if (options.highlightRegexIgnores)
			{
				mbPos += ::WideCharToMultiByte(codepage, 0, wLine.data() + highlightPos,
						static_cast<int>(rit->position() - highlightPos), NULL, 0, NULL, NULL);
				const int mbLen = ::WideCharToMultiByte(codepage, 0,
						wLine.data() + rit->position(), static_cast<int>(rit->length()), NULL, 0, NULL, NULL);

				markTextAsChanged(view, off + mbPos, mbLen, Settings.colors().blank);

				mbPos += mbLen;
			}

			highlightPos = pos = rit->position() + rit->length();
			++rit;
		}

		if (options.ignoreChangedSpaces)
		{
			intptr_t eolPos = endPos;
			while (--eolPos >= pos && (wLine[eolPos] == L'\n' || wLine[eolPos] == L'\r'));

			if (++eolPos > pos)
			{
				intptr_t p = eolPos;
				while (--p >= pos && (wLine[p] == L' ' || wLine[p] == L'\t'));

				if (++p > pos)
					hashSeed = getSectionRangeHash(hashSeed, wLine, pos, p, options);
			}

			hashSeed = getSectionRangeHash(hashSeed, wLine, eolPos, endPos, options);
		}
		else
		{
			hashSeed = getSectionRangeHash(hashSeed, wLine, pos, endPos, options);
		}
	}

	return hashSeed;
}


void getLines(DocCmpInfo& doc, const CompareOptions& options)
{
	constexpr int monitorCancelEveryXLine = 1000;

	progress_ptr& progress = ProgressDlg::Get();

	doc.lines.clear();

	if (!CallScintilla(doc.view, SCI_GETLENGTH, 0, 0))
		return;

	intptr_t linesCount = getLinesCount(doc.view);

	if (isLineEmpty(doc.view, linesCount - 1))
		--linesCount;

	if ((doc.range.len() <= 0) || (doc.range.e > linesCount))
		doc.range.e = linesCount;

	progress->SetMaxCount((doc.range.len() / monitorCancelEveryXLine) + 1);

	doc.lines.reserve(doc.range.len());

	int cancelCheckCount = monitorCancelEveryXLine;

	// Group ignore options to speed-up per-line checks
	const bool checkForIgnoredLines = !CallScintilla(doc.view, SCI_GETALLLINESVISIBLE, 0, 0) &&
		(options.ignoreFoldedLines || options.ignoreHiddenLines);
	const bool inclEmptyLinesAndEOL = !options.ignoreEOL && !options.ignoreEmptyLines;
	const bool inclEmptyLines =
		!options.ignoreEmptyLines && (!options.ignoreRegex || !options.invertRegex || options.inclRegexNomatchLines);
	const bool inclRegexEmptyLines =
		!options.ignoreEmptyLines && options.ignoreRegex && options.invertRegex && options.inclRegexNomatchLines;

	const int codepage = getCodepage(doc.view);

	for (intptr_t l = 0; l < doc.range.len(); ++l)
	{
		if (!(--cancelCheckCount))
		{
			if (!progress->Advance())
			{
				doc.lines.clear();
				return;
			}

			cancelCheckCount = monitorCancelEveryXLine;
		}

		intptr_t docLine = l + doc.range.s;

		if (checkForIgnoredLines)
		{
			if (options.ignoreFoldedLines && getNextLineAfterFold(doc.view, &docLine))
			{
				l = --docLine - doc.range.s;
				continue;
			}

			if (options.ignoreHiddenLines && isLineHidden(doc.view, docLine) && !isLineFolded(doc.view, docLine))
			{
				docLine = getUnhiddenLine(doc.view, docLine);
				l = --docLine - doc.range.s;
				continue;
			}
		}

		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEndNoEOL	= getLineEnd(doc.view, docLine);
		intptr_t lineEnd;

		if (inclEmptyLinesAndEOL)
		{
			lineEnd = lineStart + CallScintilla(doc.view, SCI_LINELENGTH, docLine, 0);
		}
		else
		{
			lineEnd = lineEndNoEOL;

			// Because of the parent 'if' that check actually means that empty lines are ignored
			if (!options.ignoreEOL)
			{
				if (lineStart == lineEnd)
					continue;
				else
					lineEnd = lineStart + CallScintilla(doc.view, SCI_LINELENGTH, docLine, 0);
			}
		}

		Line newLine {docLine, cHashSeed};

		if (lineStart < lineEnd)
		{
			std::vector<char> line = getText(doc.view, lineStart, lineEnd);

			if (options.ignoreRegex)
			{
#ifndef MULTITHREAD
				LOGD(LOG_ALGO, "Regex Ignore on line " + std::to_string(docLine + 1) +
						", view " + std::to_string(doc.view) + "\n");
#endif
				newLine.hash = getRegexIgnoreLineHash(doc.view, lineStart, newLine.hash, codepage, line, options);

				if (newLine.hash != cHashSeed || inclRegexEmptyLines)
					doc.lines.emplace_back(newLine);
			}
			else
			{
				if (options.ignoreCase)
					toLowerCase(line, codepage);

				intptr_t pos = 0;
				intptr_t endPos = lineEndNoEOL - lineStart;

				if (options.ignoreChangedSpaces)
				{
					while (pos < endPos && (line[pos] == ' ' || line[pos] == '\t'))
						++pos;

					while (--endPos >= pos && (line[endPos] == ' ' || line[endPos] == '\t'));

					++endPos;
				}

				for (; pos < endPos; ++pos)
				{
					if (options.ignoreAllSpaces && (line[pos] == ' ' || line[pos] == '\t'))
						continue;

					if (options.ignoreChangedSpaces && (line[pos] == ' ' || line[pos] == '\t'))
					{
						newLine.hash = Hash(newLine.hash, ' ');

						while (++pos < endPos && (line[pos] == ' ' || line[pos] == '\t'));

						if (pos == endPos)
							break;
					}

					newLine.hash = Hash(newLine.hash, line[pos]);
				}

				if (lineEnd > lineEndNoEOL)
				{
					endPos = lineEnd - lineStart;

					if ((options.ignoreAllSpaces || options.ignoreChangedSpaces) &&
							(line[pos] == ' ' || line[pos] == '\t'))
						while (++pos < endPos && (line[pos] == ' ' || line[pos] == '\t'));

					for (; pos < endPos; ++pos)
						newLine.hash = Hash(newLine.hash, line[pos]);
				}

				if (newLine.hash != cHashSeed || !options.ignoreEmptyLines)
					doc.lines.emplace_back(newLine);
			}
		}
		else if (inclEmptyLines)
		{
			doc.lines.emplace_back(newLine);
		}
	}
}


inline charType getCharTypeW(wchar_t letter)
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


inline void getSectionRangeWords(std::vector<Word>& words, std::vector<wchar_t>& line, intptr_t lineIdx,
		intptr_t pos, intptr_t endPos, const CompareOptions& options)
{
	if (pos >= endPos)
		return;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = line[endPos];

		line[endPos] = L'\0';

		::CharLowerW((LPWSTR)line.data() + pos);

		line[endPos] = storedChar;
	}

	charType currentWordType = getCharTypeW(line[pos]);

	Word word {lineIdx, pos, 1};

	if (options.ignoreChangedSpaces && currentWordType == charType::SPACECHAR)
		word.hash = Hash(cHashSeed, L' ');
	else
		word.hash = Hash(cHashSeed, line[pos]);

	for (; ++pos < endPos;)
	{
		const charType newWordType = getCharTypeW(line[pos]);

		if (newWordType == currentWordType)
		{
			++word.len;

			if (currentWordType != charType::SPACECHAR || !options.ignoreChangedSpaces)
				word.hash = Hash(word.hash, line[pos]);
		}
		else
		{
			if (!options.ignoreAllSpaces || currentWordType != charType::SPACECHAR)
				words.emplace_back(word);

			currentWordType = newWordType;

			word.pos = pos;
			word.len = 1;

			if (options.ignoreChangedSpaces && currentWordType == charType::SPACECHAR)
				word.hash = Hash(cHashSeed, L' ');
			else
				word.hash = Hash(cHashSeed, line[pos]);
		}
	}

	if (!options.ignoreAllSpaces || currentWordType != charType::SPACECHAR)
		words.emplace_back(word);
}


std::vector<Word> getRegexIgnoreLineWords(std::vector<wchar_t>& line, intptr_t lineIdx, const CompareOptions& options)
{
	std::vector<Word> words;

	const intptr_t len = static_cast<intptr_t>(line.size());

	if (len == 0)
		return words;

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(line.begin(), line.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		for (; rit != rend; ++rit)
			getSectionRangeWords(words, line, lineIdx, rit->position(), rit->position() + rit->length(), options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = len - 1;

		if (options.ignoreChangedSpaces && (line[pos] == L' ' || line[pos] == L'\t'))
		{
			while (++pos < endPos && (line[pos] == L' ' || line[pos] == L'\t'));

			if (pos == endPos)
				return words;
		}

		while (rit != rend)
		{
			getSectionRangeWords(words, line, lineIdx, pos, rit->position(), options);

			pos = rit->position() + rit->length();
			++rit;
		}

		--endPos;

		if (options.ignoreChangedSpaces && (line[endPos] == L' ' || line[endPos] == L'\t'))
		{
			while (--endPos >= pos && (line[endPos] == L' ' || line[endPos] == L'\t'));

			if (endPos < pos)
				return words;
		}

		getSectionRangeWords(words, line, lineIdx, pos, endPos + 1, options);
	}

	return words;
}


std::vector<Word> getLineWords(int view, int codepage, intptr_t docLine,
	const CompareOptions& options, intptr_t lineIdx = 0)
{
	std::vector<Word> words;

	const intptr_t lineStart	= getLineStart(view, docLine);
	const intptr_t lineEnd		= getLineEnd(view, docLine);

	if (lineStart >= lineEnd)
		return words;

	std::vector<char> line = getText(view, lineStart, lineEnd);

	const int len = static_cast<int>(line.size());

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

	if (options.ignoreRegex)
	{
		words = getRegexIgnoreLineWords(wLine, 0, options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces)
		{
			while (pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
				++pos;

			while (--endPos >= pos && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'));

			++endPos;
		}

		getSectionRangeWords(words, wLine, 0, pos, endPos, options);
	}

	// In case of UTF-16 or UTF-32 find words byte positions and lengths because Scintilla uses those
	if (wLen != len)
		recalculateWordPos(codepage, words, wLine);

	return words;
}


std::vector<Word> getLinesRangeWords(const DocCmpInfo& doc, const range_t& range, const CompareOptions& options)
{
	const int codepage = getCodepage(doc.view);

	std::vector<Word> words;

	for (intptr_t l = range.s; l < range.e; ++l)
	{
		std::vector<Word> lineWords = getLineWords(doc.view, codepage, doc.getDocLine(l), options, l - range.s);

		if (!lineWords.empty())
			words.insert(words.end(), lineWords.begin(), lineWords.end());
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


inline void getSectionRangeChars(std::vector<Char>& chars, std::vector<wchar_t>& sec, intptr_t pos, intptr_t endPos,
		const CompareOptions& options)
{
	if (pos >= endPos)
		return;

	if (options.ignoreCase)
	{
		const wchar_t storedChar = sec[endPos];

		sec[endPos] = L'\0';

		::CharLowerW((LPWSTR)sec.data() + pos);

		sec[endPos] = storedChar;
	}

	for (; pos < endPos; ++pos)
	{
		const charType typeOfChar = getCharTypeW(sec[pos]);

		if (options.ignoreAllSpaces && typeOfChar == charType::SPACECHAR)
			continue;

		if (options.ignoreChangedSpaces && typeOfChar == charType::SPACECHAR)
		{
			chars.emplace_back(L' ', pos);

			while (++pos < endPos && getCharTypeW(sec[pos]) == charType::SPACECHAR);

			if (pos == endPos)
				break;
		}

		chars.emplace_back(sec[pos], pos);
	}
}


std::vector<Char> getSectionChars(int view, intptr_t secStart, intptr_t secEnd, const CompareOptions& options)
{
	std::vector<Char> chars;

	if (secStart >= secEnd)
		return chars;

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

	return chars;
}


std::vector<Char> getRegexIgnoreLineChars(int view, intptr_t lineStart, intptr_t lineEnd,
	const CompareOptions& options)
{
	std::vector<Char> chars;

	if (lineStart >= lineEnd)
		return chars;

	const int codepage = getCodepage(view);

	std::vector<char> line = getText(view, lineStart, lineEnd);

	const int len = static_cast<int>(line.size());

	const int wLen = ::MultiByteToWideChar(codepage, 0, line.data(), len, NULL, 0);

	std::vector<wchar_t> wLine(wLen);

	::MultiByteToWideChar(codepage, 0, line.data(), len, wLine.data(), wLen);

	chars.reserve(wLen - 1);

	boost::regex_iterator<std::vector<wchar_t>::iterator>		rit(wLine.begin(), wLine.end(), *options.ignoreRegex);
	const boost::regex_iterator<std::vector<wchar_t>::iterator>	rend;

	if (options.invertRegex && (rit != rend || !options.inclRegexNomatchLines))
	{
		for (; rit != rend; ++rit)
			getSectionRangeChars(chars, wLine, rit->position(), rit->position() + rit->length(), options);
	}
	else
	{
		intptr_t pos = 0;
		intptr_t endPos = wLen - 1;

		if (options.ignoreChangedSpaces && (wLine[pos] == L' ' || wLine[pos] == L'\t'))
		{
			while (++pos < endPos && (wLine[pos] == L' ' || wLine[pos] == L'\t'));

			if (pos == endPos)
				return chars;
		}

		while (rit != rend)
		{
			getSectionRangeChars(chars, wLine, pos, rit->position(), options);

			pos = rit->position() + rit->length();
			++rit;
		}

		--endPos;

		if (options.ignoreChangedSpaces && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'))
			while (--endPos >= pos && (wLine[endPos] == L' ' || wLine[endPos] == L'\t'));

		if (endPos >= pos)
			getSectionRangeChars(chars, wLine, pos, endPos + 1, options);
	}

	// In case of UTF-16 or UTF-32 find chars byte positions because Scintilla uses those
	if (wLen != len)
		recalculateCharPos(codepage, chars, wLine);

	return chars;
}


std::vector<std::vector<Char>> getLinesChars(const DocCmpInfo& doc, const diff_info& bd, intptr_t diffIdx,
	const CompareOptions& options)
{
	const intptr_t diffRangeLen = doc.diffRange(bd).len();

	std::vector<std::vector<Char>> chars(diffRangeLen);

	for (intptr_t l = 0; l < diffRangeLen; ++l)
	{
		// Don't get moved lines
		if (doc.movedRanges[diffIdx].getNextUnmoved(l))
		{
			--l;
			continue;
		}

		const intptr_t docLine		= doc.getDocLine(bd, l);
		const intptr_t lineStart	= getLineStart(doc.view, docLine);
		const intptr_t lineEnd		= getLineEnd(doc.view, docLine);

		if (lineStart < lineEnd)
		{
			if (options.ignoreRegex)
			{
				chars[l] = getRegexIgnoreLineChars(doc.view, lineStart, lineEnd, options);
			}
			else
			{
				chars[l] = getSectionChars(doc.view, lineStart, lineEnd, options);

				if (options.ignoreChangedSpaces && !chars[l].empty())
				{
					auto itr = chars[l].begin();

					for (; itr != chars[l].end() && (itr->hash == L' ' || itr->hash == L'\t'); ++itr);

					if (itr != chars[l].begin())
						chars[l].erase(chars[l].begin(), itr);

					size_t i = chars[l].size() - 1;

					for (; i >= 0 && (chars[l][i] == L' ' || chars[l][i] == L'\t'); --i);

					if (++i != chars[l].size())
						chars[l].erase(chars[l].begin() + i, chars[l].end());
				}
			}
		}
	}

	return chars;
}


void findUniqueLines(CompareInfo& cmpInfo)
{
	std::unordered_map<Line::HashType, std::vector<intptr_t>> aLinesMap;

	for (const auto& line : cmpInfo.a.lines)
	{
		auto insertPair = aLinesMap.emplace(line.hash, std::vector<intptr_t>{line.num});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.num);
	}

	for (const auto& line : cmpInfo.b.lines)
	{
		auto a = aLinesMap.find(line.hash);

		if (a != aLinesMap.end())
		{
			cmpInfo.b.nonUniqueDocLines.emplace(line.num);

			auto insertPair = cmpInfo.a.nonUniqueDocLines.emplace(a->second[0]);
			if (insertPair.second)
			{
				for (size_t j = 1; j < a->second.size(); ++j)
					cmpInfo.a.nonUniqueDocLines.emplace(a->second[j]);
			}
		}
	}
}


void findMoves(CompareInfo& cmpInfo)
{
	struct MatchingLines
	{
		intptr_t	diffIdxA	{0};
		intptr_t	offA		{0};
		intptr_t	diffIdxB	{0};
		intptr_t	offB		{0};
	};

	LOGD(LOG_ALGO, "FIND MOVES\n");

	std::unordered_map<Line::HashType, MatchingLines> uniqueDiffLines;

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	for (intptr_t bi = 0; bi < blockDiffsSize; ++bi)
	{
		const diff_info& bd = cmpInfo.blockDiffs[bi];

		for (intptr_t l = 0; l < bd.a.len(); ++l)
		{
			const Line& diffLine = cmpInfo.a.getLine(bd, l);

			// Skip empty lines (do not show blocks of empty/ignored lines as moved)
			if (diffLine.hash == cHashSeed)
				continue;

			auto insertPair = uniqueDiffLines.emplace(diffLine.hash, MatchingLines{});
			if (insertPair.first->second.diffIdxA == 0)
			{
				insertPair.first->second.diffIdxA = bi + 1;
				insertPair.first->second.offA = l + 1;
			}
			else
			{
				insertPair.first->second.diffIdxA = -1;
			}
		}

		for (intptr_t l = 0; l < bd.b.len(); ++l)
		{
			const Line& diffLine = cmpInfo.b.getLine(bd, l);

			// Skip empty lines (do not show blocks of empty/ignored lines as moved)
			if (diffLine.hash == cHashSeed)
				continue;

			auto insertPair = uniqueDiffLines.emplace(diffLine.hash, MatchingLines{});
			if (insertPair.first->second.diffIdxB == 0)
			{
				insertPair.first->second.diffIdxB = bi + 1;
				insertPair.first->second.offB = l + 1;
			}
			else
			{
				insertPair.first->second.diffIdxB = -1;
			}
		}
	}

	for (const auto& ul : uniqueDiffLines)
	{
		if (ul.second.diffIdxA <= 0 || ul.second.diffIdxB <= 0)
			continue;

		if (cmpInfo.a.movedRanges[ul.second.diffIdxA - 1].contain(ul.second.offA - 1) ||
			cmpInfo.b.movedRanges[ul.second.diffIdxB - 1].contain(ul.second.offB - 1))
			continue;

		const diff_info& diffA = cmpInfo.blockDiffs[ul.second.diffIdxA - 1];
		const diff_info& diffB = cmpInfo.blockDiffs[ul.second.diffIdxB - 1];

		intptr_t startA = ul.second.offA - 2;
		intptr_t startB = ul.second.offB - 2;

		while (startA >= 0 && startB >= 0 && cmpInfo.a.getLine(diffA, startA) == cmpInfo.b.getLine(diffB, startB))
		{
			--startA;
			--startB;
		}

		intptr_t endA = ul.second.offA;
		intptr_t endB = ul.second.offB;

		while (endA < diffA.a.len() && endB < diffB.b.len() &&
				cmpInfo.a.getLine(diffA, endA) == cmpInfo.b.getLine(diffB, endB))
		{
			++endA;
			++endB;
		}

		cmpInfo.a.movedRanges[ul.second.diffIdxA - 1].emplace_back(startA + 1, endA);
		cmpInfo.b.movedRanges[ul.second.diffIdxB - 1].emplace_back(startB + 1, endB);

		LOGD(LOG_ALGO,
			"\tA range: [" + std::to_string(cmpInfo.a.getDocLine(diffA, startA + 1)) + ", " +
				std::to_string(cmpInfo.a.getDocLine(diffA, endA)) +
			")   B range: [" + std::to_string(cmpInfo.b.getDocLine(diffB, startB + 1)) + ", " +
				std::to_string(cmpInfo.b.getDocLine(diffB, endB)) + ")\n");
	}
}


inline intptr_t matchBeginEnd(ChangedLine& changedLineA, ChangedLine& changedLineB,
		const std::vector<Char>& secA, const std::vector<Char>& secB,
		intptr_t offA, intptr_t offB, intptr_t endA, intptr_t endB,
		std::function<bool(const wchar_t)>&& charFilter_fn)
{
	intptr_t minSecSize = std::min(secA.size(), secB.size());

	intptr_t startMatch = 0;
	while ((minSecSize > startMatch) && (secA[startMatch] == secB[startMatch]) && charFilter_fn(secA[startMatch].hash))
		++startMatch;

	minSecSize -= startMatch;

	intptr_t endMatch = 0;
	while ((minSecSize > endMatch) &&
			(secA[secA.size() - endMatch - 1] == secB[secB.size() - endMatch - 1]) &&
			charFilter_fn(secA[secA.size() - endMatch - 1].hash))
		++endMatch;

	changed_range_t change;

	if ((intptr_t)secA.size() > startMatch + endMatch)
	{
		change.s = offA;
		if (startMatch)
			change.s += secA[startMatch].pos;

		change.e = (endMatch ? secA[secA.size() - endMatch - 1].pos + 1 + offA : endA);

		changedLineA.changes.emplace_back(change);
	}

	if ((intptr_t)secB.size() > startMatch + endMatch)
	{
		change.s = offB;
		if (startMatch)
			change.s += secB[startMatch].pos;

		change.e = (endMatch ? secB[secB.size() - endMatch - 1].pos + 1 + offB : endB);

		changedLineB.changes.emplace_back(change);
	}

	return (startMatch + endMatch);
}


void findSubLineMoves(int view1, intptr_t docLine1, int view2, intptr_t docLine2,
	std::vector<changed_range_t>& line_changes1, std::vector<changed_range_t>& line_changes2)
{
	const std::vector<char> lineA = getText(view1, getLineStart(view1, docLine1), getLineEnd(view1, docLine1));
	const std::vector<char> lineB = getText(view2, getLineStart(view2, docLine2), getLineEnd(view2, docLine2));

	for (auto lc1 = line_changes1.begin(); lc1 != line_changes1.end(); lc1++)
	{
		bool sameSectionFound = false;

		for (auto lc = line_changes1.begin(); lc != line_changes1.end(); lc++)
		{
			if (lc->s == lc1->s || lc->moved)
				continue;

			if (lc->len() == lc1->len() && std::equal(&lineA[lc->s], &lineA[lc->e], &lineA[lc1->s]))
			{
				sameSectionFound = true;
				break;
			}
		}

		if (sameSectionFound)
			continue;

		auto lc2same = line_changes2.end();

		for (auto lc2 = line_changes2.begin(); lc2 != line_changes2.end(); lc2++)
		{
			if (lc2->moved)
				continue;

			if (lc2->len() == lc1->len() && std::equal(&lineB[lc2->s], &lineB[lc2->e], &lineA[lc1->s]))
			{
				if (lc2same == line_changes2.end())
				{
					lc2same = lc2;
				}
				else
				{
					lc2same = line_changes2.end();
					break;
				}
			}
		}

		if (lc2same != line_changes2.end())
		{
			lc1->moved = true;
			lc2same->moved = true;
		}
	}
}


void compareLines(CompareInfo& cmpInfo, intptr_t diffIdx, const std::map<intptr_t, intptr_t>& lineMappings,
	const CompareOptions& options)
{
	DocCmpInfo&			a = cmpInfo.a;
	DocCmpInfo&			b = cmpInfo.b;
	const diff_info&	bd = cmpInfo.blockDiffs[diffIdx];

	for (const auto& lm : lineMappings)
	{
		const intptr_t lineA = a.getDocLine(bd, lm.second);
		const intptr_t lineB = b.getDocLine(bd, lm.first);

		LOGD(LOG_ALGO, "Compare Lines " + std::to_string(lineA + 1) + " and " + std::to_string(lineB + 1) + "\n");

		const std::vector<Word> lineWordsA = getLineWords(a.view, getCodepage(a.view), lineA, options);
		const std::vector<Word> lineWordsB = getLineWords(b.view, getCodepage(b.view), lineB, options);

		// First use word granularity (find matching words) for better precision
		const auto lineDiffs = DiffCalc<Word>(lineWordsA, lineWordsB)(true);
		const intptr_t lineDiffsSize = static_cast<intptr_t>(lineDiffs.size());

		PRINT_DIFFS("WORD DIFFS", lineDiffs);

		a.changedLines[diffIdx].emplace_back(lm.second);
		b.changedLines[diffIdx].emplace_back(lm.first);

		auto& changedLineA = a.changedLines[diffIdx].back();
		auto& changedLineB = b.changedLines[diffIdx].back();

		const intptr_t linePosA = getLineStart(a.view, lineA);
		const intptr_t linePosB = getLineStart(b.view, lineB);

		intptr_t lineLenA = 0;
		intptr_t lineLenB = 0;

		for (const auto& word : lineWordsA)
			lineLenA += word.len;

		for (const auto& word : lineWordsB)
			lineLenB += word.len;

		intptr_t totalLineMatchLen = lineLenA + lineLenB;

		for (intptr_t i = 0; i < lineDiffsSize; ++i)
		{
			const auto& ld = lineDiffs[i];

			// Resolve words mismatched pairs to find possible sub-word similarities
			if (ld.is_replacement())
			{
				intptr_t offA = lineWordsA[ld.a.s].pos;
				intptr_t endA = lineWordsA[ld.a.e - 1].pos + lineWordsA[ld.a.e - 1].len;

				intptr_t offB = lineWordsB[ld.b.s].pos;
				intptr_t endB = lineWordsB[ld.b.e - 1].pos + lineWordsB[ld.b.e - 1].len;

				const std::vector<Char> secA = getSectionChars(a.view, linePosA + offA, linePosA + endA, options);
				const std::vector<Char> secB = getSectionChars(b.view, linePosB + offB, linePosB + endB, options);

				if (options.detectCharDiffs)
				{
					LOGD(LOG_ALGO, "Compare Sections " +
							std::to_string(offA + 1) + " to " + std::to_string(endA + 1) + " and " +
							std::to_string(offB + 1) + " to " + std::to_string(endB + 1) + "\n");

					// Compare changed words
					const auto sectionDiffs = DiffCalc<Char>(secA, secB)(true);

					PRINT_DIFFS("CHAR DIFFS", sectionDiffs);

					for (const auto& sd : sectionDiffs)
					{
						if (sd.a.len())
						{
							changed_range_t change;

							change.s = secA[sd.a.s].pos + offA;
							change.e = secA[sd.a.e - 1].pos + offA + 1;

							changedLineA.changes.emplace_back(change);

							totalLineMatchLen -= change.len();
						}
						if (sd.b.len())
						{
							changed_range_t change;

							change.s = secB[sd.b.s].pos + offB;
							change.e = secB[sd.b.e - 1].pos + offB + 1;

							changedLineB.changes.emplace_back(change);

							totalLineMatchLen -= change.len();
						}
					}
				}
				// Always match non-alphabetical characters in the beginning and at the end of changed sections
				else
				{
					const intptr_t matchBEcount =
							matchBeginEnd(changedLineA, changedLineB, secA, secB, offA, offB, endA, endB,
								[](const wchar_t ch) { return (getCharTypeW(ch) != charType::ALPHANUMCHAR); });

					totalLineMatchLen -= ((endA - offA) + (endB - offB) - (2 * matchBEcount));
				}
			}
			else if (ld.a.len())
			{
				changed_range_t change;

				change.s = lineWordsA[ld.a.s].pos;
				change.e = lineWordsA[ld.a.e - 1].pos + lineWordsA[ld.a.e - 1].len;

				changedLineA.changes.emplace_back(change);

				totalLineMatchLen -= change.len();
			}
			else
			{
				changed_range_t change;

				change.s = lineWordsB[ld.b.s].pos;
				change.e = lineWordsB[ld.b.e - 1].pos + lineWordsB[ld.b.e - 1].len;

				changedLineB.changes.emplace_back(change);

				totalLineMatchLen -= change.len();
			}
		}

		// Not enough portion of the lines matches - consider them totally different
		if (((totalLineMatchLen * 100) / (lineLenA + lineLenB)) < options.changedResemblPercent)
		{
			a.changedLines[diffIdx].pop_back();
			b.changedLines[diffIdx].pop_back();
		}
		else if (options.detectSubLineMoves)
		{
			if (!changedLineA.changes.empty() && !changedLineB.changes.empty())
				findSubLineMoves(a.view, lineA, b.view, lineB, changedLineA.changes, changedLineB.changes);
		}
	}
}


std::vector<std::set<LinesConv>> getOrderedConvergence(const CompareInfo& cmpInfo, intptr_t diffIdx,
	const CompareOptions& options)
{
	const DocCmpInfo& a	= cmpInfo.a;
	const DocCmpInfo& b	= cmpInfo.b;
	const diff_info& bd	= cmpInfo.blockDiffs[diffIdx];

	const std::vector<std::vector<Char>> chunkA = getLinesChars(a, bd, diffIdx, options);
	const std::vector<std::vector<Char>> chunkB = getLinesChars(b, bd, diffIdx, options);

	const intptr_t linesCountA = static_cast<intptr_t>(chunkA.size());
	const intptr_t linesCountB = static_cast<intptr_t>(chunkB.size());

	std::vector<std::vector<Word>> wordsB(linesCountB);

	if (!options.detectCharDiffs || !options.ignoreAllSpaces)
	{
		for (intptr_t lineB = 0; lineB < linesCountB; ++lineB)
			if (!chunkB[lineB].empty())
				wordsB[lineB] = getLineWords(b.view, getCodepage(b.view), b.getDocLine(bd, lineB), options);
	}

	std::vector<std::set<LinesConv>> linesAConvergence(linesCountA);
	std::vector<std::set<LinesConv>> linesBConvergence(linesCountB);

	progress_ptr& progress = ProgressDlg::Get();

	intptr_t linesProgress = 0;

	for (intptr_t lineA = 0; lineA < linesCountA; ++lineA)
	{
		if (chunkA[lineA].empty())
		{
			linesProgress += linesCountB;
			continue;
		}

		std::vector<Word> wordsA;

		for (intptr_t lineB = 0; lineB < linesCountB; ++lineB)
		{
			if (chunkB[lineB].empty())
			{
				++linesProgress;
				continue;
			}

			const intptr_t minSize = std::min(chunkA[lineA].size(), chunkB[lineB].size());
			const intptr_t maxSize = std::max(chunkA[lineA].size(), chunkB[lineB].size());

			if (((minSize * 100) / maxSize) < options.changedResemblPercent)
			{
				++linesProgress;
				continue;
			}

			intptr_t matchLen = chunkA[lineA].size() + chunkB[lineB].size();
			intptr_t longestMatch = 0;

			if (!options.detectCharDiffs || !options.ignoreAllSpaces)
			{
				if (wordsA.empty())
					wordsA = getLineWords(a.view, getCodepage(a.view), a.getDocLine(bd, lineA), options);

				const auto wordDiffs = DiffCalc<Word>(wordsA, wordsB[lineB],
						std::bind(&ProgressDlg::IsCancelled, progress))();

				if (progress->IsCancelled())
					return {};

				const intptr_t wordDiffsSize = static_cast<intptr_t>(wordDiffs.size());

				for (intptr_t i = 0; i < wordDiffsSize; ++i)
				{
					if (wordDiffs[i].a.len())
					{
						for (intptr_t n = 0; n < wordDiffs[i].a.len(); ++n)
							matchLen -= wordsA[wordDiffs[i].a.s + n].len;
					}
					if (wordDiffs[i].b.len())
					{
						for (intptr_t n = 0; n < wordDiffs[i].b.len(); ++n)
							matchLen -= wordsB[lineB][wordDiffs[i].b.s + n].len;
					}
				}

				// if (matchLen > longestMatch)
					// longestMatch = matchLen;
			}
			else
			{
				const auto charDiffs = DiffCalc<Char>(chunkA[lineA], chunkB[lineB],
						std::bind(&ProgressDlg::IsCancelled, progress))();

				if (progress->IsCancelled())
					return {};

				const intptr_t charDiffsSize = static_cast<intptr_t>(charDiffs.size());

				for (intptr_t i = 0; i < charDiffsSize; ++i)
					matchLen -= (charDiffs[i].a.len() + charDiffs[i].b.len());

				// if (matchLen > longestMatch)
					// longestMatch = matchLen;
			}

			if (((matchLen * 100) / static_cast<intptr_t>(chunkA[lineA].size() + chunkB[lineB].size())) >=
				options.changedResemblPercent)
			{
				const float conv =
						(static_cast<float>(matchLen) * 100) / (chunkA[lineA].size() + chunkB[lineB].size()) +
						(static_cast<float>(longestMatch) * 100) / (chunkA[lineA].size() + chunkB[lineB].size());

				if (!progress->Advance(linesProgress + 1))
					return {};

				linesProgress = 0;

				bool addL1C = false;

				if (linesBConvergence[lineB].empty() || (conv == linesBConvergence[lineB].begin()->conv))
				{
					LOGD(LOG_CHANGE_ALGO, "Add L2: " + std::to_string(lineA) + ", " + std::to_string(lineB) +
							", " + std::to_string(conv) + "\n");

					linesBConvergence[lineB].emplace(conv, lineA, lineB);
					addL1C = true;
				}
				else if (conv > linesBConvergence[lineB].begin()->conv)
				{
					LOGD(LOG_CHANGE_ALGO, "Replace L2: " + std::to_string(lineA) + ", " +
							std::to_string(lineB) + ", " + std::to_string(conv) + "\n");

					for (const auto& l2c : linesBConvergence[lineB])
					{
						auto& l1c = linesAConvergence[l2c.lineA];

						for (auto l1cI = l1c.begin(); l1cI != l1c.end(); ++l1cI)
						{
							LOGD(LOG_CHANGE_ALGO, "Check L1: " + std::to_string(l2c.lineA) + ", " +
									std::to_string(l1cI->lineB) + ", " +
									std::to_string(l1cI->conv) + "\n");

							if (l1cI->lineB == lineB)
							{
								LOGD(LOG_CHANGE_ALGO, "Erase\n");

								l1c.erase(l1cI);
								break;
							}
						}
					}

					linesBConvergence[lineB].clear();
					linesBConvergence[lineB].emplace(conv, lineA, lineB);
					addL1C = true;
				}

				if (addL1C)
				{
					if (linesAConvergence[lineA].empty() || (conv == linesAConvergence[lineA].begin()->conv))
					{
						linesAConvergence[lineA].emplace(conv, lineA, lineB);
					}
					else if (conv > linesAConvergence[lineA].begin()->conv)
					{
						linesAConvergence[lineA].clear();
						linesAConvergence[lineA].emplace(conv, lineA, lineB);
					}
				}
			}
			else
			{
				if (!progress->Advance(linesProgress + 1))
					return {};

				linesProgress = 0;
			}
		}
	}

	return linesAConvergence;
}


bool findChangesLineByLine(CompareInfo& cmpInfo, intptr_t diffIdx, const CompareOptions& options)
{
	std::vector<std::set<LinesConv>> orderedLinesConvergence = getOrderedConvergence(cmpInfo, diffIdx, options);

	if (ProgressDlg::Get()->IsCancelled())
		return false;

#ifdef DLOG
	DocCmpInfo&			a = cmpInfo.a;
	DocCmpInfo&			b = cmpInfo.b;
	const diff_info&	bd = cmpInfo.blockDiffs[diffIdx];

	for (const auto& oc : orderedLinesConvergence)
	{
		if (!oc.empty())
			LOGD(LOG_ALGO, "Best Matching Lines: " +
					std::to_string(a.getDocLine(bd, oc.begin()->lineA) + 1) + " and " +
					std::to_string(b.getDocLine(bd, oc.begin()->lineB) + 1) + ", Conv (" +
					std::to_string(oc.begin()->conv) + ")\n");
	}
#endif

	std::map<intptr_t, intptr_t> bestLineMappings; // lineB -> lineA
	{
		std::vector<std::map<intptr_t, intptr_t>> groupedLines;

		for (const auto& oc : orderedLinesConvergence)
		{
			if (oc.empty())
				continue;

			if (groupedLines.empty())
			{
				auto ocItr = oc.begin();

				groupedLines.emplace_back();
				groupedLines.back().emplace(ocItr->lineB, ocItr->lineA);

				continue;
			}

			intptr_t addToIdx = -1;

			for (auto ocItr = oc.begin(); ocItr != oc.end(); ++ocItr)
			{
				for (intptr_t i = 0; i < static_cast<intptr_t>(groupedLines.size()); ++i)
				{
					const auto& gl = groupedLines[i];

					if ((ocItr->lineB) > (gl.rbegin()->first))
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
					gl.emplace_hint(gl.end(), ocItr->lineB, ocItr->lineA);

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

				auto glResItr = gl.emplace(ocrItr->lineB, ocrItr->lineA);

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
						std::to_string(a.getDocLine(bd, ocrItr->lineA) + 1) + " - " +
						std::to_string(b.getDocLine(bd, ocrItr->lineB) + 1) + "\n");
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

	compareLines(cmpInfo, diffIdx, bestLineMappings, options);

	return true;
}


inline intptr_t addLineChange(const std::vector<Word>& wordsRange, intptr_t startWord, intptr_t endWord,
	std::vector<ChangedLine>& changedLines, intptr_t lastChangedLineIdx)
{
	if (wordsRange[startWord].lineIdx != wordsRange[endWord].lineIdx)
	{
		if (startWord > 0 && wordsRange[startWord].lineIdx == wordsRange[startWord - 1].lineIdx)
		{
			intptr_t lineEndWord = startWord;

			while (wordsRange[++lineEndWord].lineIdx == wordsRange[startWord].lineIdx);
			--lineEndWord;

			if (lastChangedLineIdx != wordsRange[startWord].lineIdx)
			{
				lastChangedLineIdx = wordsRange[startWord].lineIdx;
				changedLines.emplace_back(lastChangedLineIdx);
			}

			changedLines.back().changes.emplace_back(wordsRange[startWord].pos,
					wordsRange[lineEndWord].pos + wordsRange[lineEndWord].len);
		}

		if (static_cast<size_t>(endWord + 1) < wordsRange.size() &&
			wordsRange[endWord].lineIdx == wordsRange[endWord + 1].lineIdx)
		{
			startWord = endWord;

			while (wordsRange[--startWord].lineIdx == wordsRange[endWord].lineIdx);
			++startWord;

			lastChangedLineIdx = wordsRange[endWord].lineIdx;
			changedLines.emplace_back(lastChangedLineIdx);
			changedLines.back().changes.emplace_back(wordsRange[startWord].pos,
					wordsRange[endWord].pos + wordsRange[endWord].len);
		}

	}
	else if ((startWord > 0 && wordsRange[startWord].lineIdx == wordsRange[startWord - 1].lineIdx) ||
		(static_cast<size_t>(endWord + 1) < wordsRange.size() &&
		wordsRange[endWord].lineIdx == wordsRange[endWord + 1].lineIdx))
	{
		if (lastChangedLineIdx != wordsRange[startWord].lineIdx)
		{
			lastChangedLineIdx = wordsRange[startWord].lineIdx;
			changedLines.emplace_back(lastChangedLineIdx);
		}

		changedLines.back().changes.emplace_back(wordsRange[startWord].pos,
				wordsRange[endWord].pos + wordsRange[endWord].len);
	}

	return lastChangedLineIdx;
}


bool findChanges(CompareInfo& cmpInfo, intptr_t diffIdx, const CompareOptions& options)
{
	const std::vector<Word> wordsRangeA = getLinesRangeWords(cmpInfo.a, cmpInfo.blockDiffs[diffIdx].a, options);
	const std::vector<Word> wordsRangeB = getLinesRangeWords(cmpInfo.b, cmpInfo.blockDiffs[diffIdx].b, options);

	if (wordsRangeA.empty() || wordsRangeB.empty())
		return true;

	const auto rangeDiffs = DiffCalc<Word>(wordsRangeA, wordsRangeB)(true);

	if (ProgressDlg::Get()->IsCancelled())
		return false;

	PRINT_DIFFS("WORD DIFFS", rangeDiffs);

	auto& changedLinesA = cmpInfo.a.changedLines[diffIdx];
	auto& changedLinesB = cmpInfo.b.changedLines[diffIdx];

	intptr_t lastChangedLineIdxA = -1;
	intptr_t lastChangedLineIdxB = -1;

	for (const auto& rd : rangeDiffs)
	{
		if (rd.a.len())
		{
			lastChangedLineIdxA = addLineChange(wordsRangeA, rd.a.s, rd.a.e - 1, changedLinesA, lastChangedLineIdxA);
		}
		else
		{
			const intptr_t startWord = rd.a.s > 0 ? rd.a.s - 1 : 0;

			if (lastChangedLineIdxA != wordsRangeA[startWord].lineIdx)
			{
				lastChangedLineIdxA = wordsRangeA[startWord].lineIdx;
				changedLinesA.emplace_back(lastChangedLineIdxA);
			}
		}

		if (rd.b.len())
		{
			lastChangedLineIdxB = addLineChange(wordsRangeB, rd.b.s, rd.b.e - 1, changedLinesB, lastChangedLineIdxB);
		}
		else
		{
			const intptr_t startWord = rd.b.s > 0 ? rd.b.s - 1 : 0;

			if (lastChangedLineIdxB != wordsRangeB[startWord].lineIdx)
			{
				lastChangedLineIdxB = wordsRangeB[startWord].lineIdx;
				changedLinesB.emplace_back(lastChangedLineIdxB);
			}
		}
	}

	// Temp guard
	if (changedLinesA.size() != changedLinesB.size())
	{
		// std::string msg = "Diff index " + std::to_string(diffIdx + 1) + " - changed lines: " +
			// std::to_string(changedLinesA.size()) + " vs. " + std::to_string(changedLinesB.size());

		// ::MessageBoxA(nppData._nppHandle, msg.c_str(), "ComparePlus", MB_OK | MB_ICONWARNING);

		changedLinesA.clear();
		changedLinesB.clear();

		findChangesLineByLine(cmpInfo, diffIdx, options);
		return true;

		while (changedLinesA.size() > changedLinesB.size())
			changedLinesA.pop_back();
		while (changedLinesB.size() > changedLinesA.size())
			changedLinesB.pop_back();
	}

	if (options.detectSubLineMoves)
	{
		const size_t changedLinesSize =
				changedLinesA.size() > changedLinesB.size() ? changedLinesB.size() : changedLinesA.size();

		for (size_t i = 0; i < changedLinesSize; ++i)
		{
			if (!changedLinesA[i].changes.empty() && !changedLinesB[i].changes.empty())
				// NOTE: Optimize this making new function directly using already gotten ranges words!
				findSubLineMoves(
					cmpInfo.a.view, cmpInfo.a.getDocLine(cmpInfo.blockDiffs[diffIdx], changedLinesA[i].idx),
					cmpInfo.b.view, cmpInfo.b.getDocLine(cmpInfo.blockDiffs[diffIdx], changedLinesB[i].idx),
					changedLinesA[i].changes, changedLinesB[i].changes);
		}
	}

	return true;
}


void findSubBlockDiffs(CompareInfo& cmpInfo, const CompareOptions& options)
{
	progress_ptr& progress = ProgressDlg::Get();

	std::vector<intptr_t> changedBlockIdx;
	intptr_t changedProgressCount = 0;

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	// Get changed blocks to sub-compare
	for (intptr_t i = 0; i < blockDiffsSize; ++i)
	{
		if (cmpInfo.blockDiffs[i].is_replacement())
		{
			changedBlockIdx.emplace_back(i);
			changedProgressCount += cmpInfo.blockDiffs[i].a.len() * cmpInfo.blockDiffs[i].b.len();
		}
	}

	progress->SetMaxCount(changedProgressCount);

	if (changedProgressCount > 10000)
		progress->Show();

#ifdef MULTITHREAD // Do multithreaded block compares

	const int threadsCount = std::thread::hardware_concurrency() - 2;

	if (threadsCount > 0)
	{
		LOGD(LOG_ALL, "Changes detection running on " + std::to_string(threadsCount + 1) + " threads\n");

		std::atomic<size_t> blockIdx {0};

		auto threadFn =
			[&]()
			{
				for (size_t i = blockIdx++; i < changedBlockIdx.size(); i = blockIdx++)
				{
					if (!findChanges(cmpInfo, changedBlockIdx[i], options))
						return;
				}
			};

		std::vector<std::thread> threads(threadsCount);

		for (auto& th : threads)
			th = std::thread(threadFn);

		threadFn();

		for (auto& th : threads)
			th.join();
	}
	else
	{
		LOGD(LOG_ALL, "Changes detection running on 1 thread\n");

		for (intptr_t i : changedBlockIdx)
		{
			if (!findChanges(cmpInfo, i, options))
				break;
		}
	}

#else // Do block compares in single thread

	for (intptr_t i : changedBlockIdx)
	{
		if (!findChanges(cmpInfo, i, options))
			break;
	}

#endif // MULTITHREAD
}


inline void markLine(int view, intptr_t line, int mark)
{
	CallScintilla(view, SCI_ENSUREVISIBLE, line, 0);
	CallScintilla(view, SCI_SHOWLINES, line, line);
	CallScintilla(view, SCI_MARKERADDSET, line, mark);
}


inline void markLinesBetween(int view, intptr_t& prevDocLine, intptr_t docLine, int mark)
{
	while (++prevDocLine < docLine)
		markLine(view, prevDocLine, mark);
}


void markSection(const DocCmpInfo& doc, intptr_t bi, intptr_t diffOff, const CompareOptions& options)
{
	const int diffMaskLocal =
			(doc.diffMask == MARKER_MASK_ADDED) ? MARKER_MASK_ADDED_LOCAL : MARKER_MASK_REMOVED_LOCAL;
	const intptr_t rangeEnd = doc.range.e - diffOff;

	intptr_t prevDocLine = doc.getDocLine(doc.range.s) - 1;

	for (intptr_t i = doc.range.s - diffOff, l = doc.range.s; i < rangeEnd; ++i, ++l)
	{
		intptr_t docLine	= doc.getDocLine(l);
		intptr_t movedLen	= doc.movedRanges[bi].rangeLen(i);

		while (movedLen == 0)
		{
			const int mark = (doc.nonUniqueDocLines.find(docLine) ==
					doc.nonUniqueDocLines.end()) ? doc.diffMask : diffMaskLocal;

			markLine(doc.view, docLine, mark);

			if (!options.neverMarkIgnored)
				markLinesBetween(doc.view, prevDocLine, docLine, doc.diffMask);

			if (++i >= rangeEnd)
				return;

			docLine = doc.getDocLine(++l);
			movedLen = doc.movedRanges[bi].rangeLen(i);
		}

		if (!options.neverMarkIgnored)
			markLinesBetween(doc.view, prevDocLine, docLine, doc.diffMask);

		if (i + movedLen > rangeEnd)
			movedLen = rangeEnd - i;

		if (movedLen == 1)
		{
			markLine(doc.view, docLine, MARKER_MASK_MOVED_SINGLE);
		}
		else
		{
			markLine(doc.view, docLine, MARKER_MASK_MOVED_BEGIN);

			i += --movedLen;

			const intptr_t endL = l + movedLen;

			while (++l < endL)
			{
				docLine = doc.getDocLine(l);
				markLine(doc.view, docLine, MARKER_MASK_MOVED_MID);

				if (!options.neverMarkIgnored)
					markLinesBetween(doc.view, prevDocLine, docLine, doc.diffMask);
			}

			docLine = doc.getDocLine(l);
			markLine(doc.view, docLine, MARKER_MASK_MOVED_END);

			if (!options.neverMarkIgnored)
				markLinesBetween(doc.view, prevDocLine, docLine, doc.diffMask);
		}
	}
}


void markLineDiffs(const CompareInfo& cmpInfo, intptr_t bi, intptr_t ci)
{
	const auto& changedLineA = cmpInfo.a.changedLines[bi][ci];
	const auto& changedLineB = cmpInfo.b.changedLines[bi][ci];

	intptr_t line = cmpInfo.a.getDocLine(cmpInfo.blockDiffs[bi], changedLineA.idx);
	intptr_t linePos = getLineStart(cmpInfo.a.view, line);
	int color = (cmpInfo.a.diffMask == MARKER_MASK_ADDED) ?
			Settings.colors().added_part : Settings.colors().removed_part;

	for (const auto& change : changedLineA.changes)
		markTextAsChanged(cmpInfo.a.view, linePos + change.s, change.len(),
						change.moved ? Settings.colors().moved_part : color);

	markLine(cmpInfo.a.view, line, cmpInfo.a.nonUniqueDocLines.find(line) == cmpInfo.a.nonUniqueDocLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);

	line = cmpInfo.b.getDocLine(cmpInfo.blockDiffs[bi], changedLineB.idx);
	linePos = getLineStart(cmpInfo.b.view, line);
	color = (cmpInfo.b.diffMask == MARKER_MASK_ADDED) ?
			Settings.colors().added_part : Settings.colors().removed_part;

	for (const auto& change : changedLineB.changes)
		markTextAsChanged(cmpInfo.b.view, linePos + change.s, change.len(),
						change.moved ? Settings.colors().moved_part : color);

	markLine(cmpInfo.b.view, line, cmpInfo.b.nonUniqueDocLines.find(line) == cmpInfo.b.nonUniqueDocLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);
}


inline void markReplacedBlockDiffRange(CompareInfo& cmpInfo, intptr_t bi, const CompareOptions& options,
	CompareSummary& summary)
{
	if (cmpInfo.a.range.len() && cmpInfo.b.range.len())
	{
		AlignmentPair alignPair;

		alignPair.main.diffMask	= cmpInfo.a.diffMask;
		alignPair.main.line		= cmpInfo.a.getDocLine(cmpInfo.a.range.s);
		alignPair.sub.diffMask	= cmpInfo.b.diffMask;
		alignPair.sub.line		= cmpInfo.b.getDocLine(cmpInfo.b.range.s);

		summary.alignmentInfo.emplace_back(alignPair);

		if (options.neverMarkIgnored)
		{
			std::vector<intptr_t> bdAlignIdxsA;

			for (intptr_t l = cmpInfo.a.range.s + 1; l < cmpInfo.a.range.e; ++l)
			{
				if (cmpInfo.a.getDocLine(l) - cmpInfo.a.getDocLine(l - 1) > 1)
					bdAlignIdxsA.emplace_back(l);
			}

			if (!bdAlignIdxsA.empty())
			{
				auto alignItrA = bdAlignIdxsA.begin();

				for (intptr_t l = cmpInfo.b.range.s + 1; l < cmpInfo.b.range.e; ++l)
				{
					if (cmpInfo.b.getDocLine(l) - cmpInfo.b.getDocLine(l - 1) > 1)
					{
						alignPair.main.line	= cmpInfo.a.getDocLine(*alignItrA);
						alignPair.sub.line	= cmpInfo.b.getDocLine(l);

						summary.alignmentInfo.emplace_back(alignPair);

						if (++alignItrA == bdAlignIdxsA.end())
							break;
					}
				}
			}
		}
	}

	if (cmpInfo.a.range.len())
		markSection(cmpInfo.a, bi, cmpInfo.blockDiffs[bi].a.s, options);

	if (cmpInfo.b.range.len())
		markSection(cmpInfo.b, bi, cmpInfo.blockDiffs[bi].b.s, options);
}


bool markAllDiffs(CompareInfo& cmpInfo, const CompareOptions& options, CompareSummary& summary)
{
	clearWindow(MAIN_VIEW, false);
	clearWindow(SUB_VIEW, false);

	progress_ptr& progress = ProgressDlg::Get();

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	progress->SetMaxCount(blockDiffsSize);

	intptr_t alignIdxA = 0;
	intptr_t alignIdxB = 0;

	AlignmentPair alignPair;

	for (intptr_t bi = 0; bi < blockDiffsSize; ++bi)
	{
		const diff_info& bd = cmpInfo.blockDiffs[bi];
		intptr_t matchLen = bd.a.distance_from(alignIdxA);

		if (matchLen > 0)
		{
			summary.match += matchLen;

			alignPair.main.diffMask	= 0;
			alignPair.sub.diffMask	= 0;

			// Align all pairs of matching lines
			while (matchLen--)
			{
				alignPair.main.line	= cmpInfo.a.getDocLine(alignIdxA++);
				alignPair.sub.line	= cmpInfo.b.getDocLine(alignIdxB++);

				summary.alignmentInfo.emplace_back(alignPair);
			}
		}

		if (bd.is_replacement())
		{
			const intptr_t changedCount = static_cast<intptr_t>(cmpInfo.a.changedLines[bi].size());

			alignIdxA = bd.a.s;
			alignIdxB = bd.b.s;

			for (intptr_t ci = 0; ci < changedCount; ++ci)
			{
				cmpInfo.a.range.s = alignIdxA;
				cmpInfo.a.range.e = bd.a.s + cmpInfo.a.changedLines[bi][ci].idx;
				cmpInfo.b.range.s = alignIdxB;
				cmpInfo.b.range.e = bd.b.s + cmpInfo.b.changedLines[bi][ci].idx;

				markReplacedBlockDiffRange(cmpInfo, bi, options, summary);

				alignIdxA = cmpInfo.a.range.e;
				alignIdxB = cmpInfo.b.range.e;

				alignPair.main.diffMask	= MARKER_MASK_CHANGED;
				alignPair.main.line		= cmpInfo.a.getDocLine(alignIdxA++);

				alignPair.sub.diffMask	= MARKER_MASK_CHANGED;
				alignPair.sub.line		= cmpInfo.b.getDocLine(alignIdxB++);

				summary.alignmentInfo.emplace_back(alignPair);

				markLineDiffs(cmpInfo, bi, ci);
			}

			cmpInfo.a.range.s = alignIdxA;
			cmpInfo.a.range.e = bd.a.e;
			cmpInfo.b.range.s = alignIdxB;
			cmpInfo.b.range.e = bd.b.e;

			markReplacedBlockDiffRange(cmpInfo, bi, options, summary);

			alignIdxA = bd.a.e;
			alignIdxB = bd.b.e;

			const intptr_t movedLinesA = cmpInfo.a.movedRanges[bi].totalLinesCount();
			const intptr_t movedLinesB = cmpInfo.b.movedRanges[bi].totalLinesCount();

			const intptr_t newLinesA = bd.a.len() - changedCount - movedLinesA;
			const intptr_t newLinesB = bd.b.len() - changedCount - movedLinesB;

			summary.diffLines	+= newLinesA + newLinesB + changedCount;
			summary.changed		+= changedCount;
			summary.moved		+= movedLinesA + movedLinesB;

			if (cmpInfo.a.diffMask == MARKER_MASK_ADDED)
			{
				summary.added	+= newLinesA;
				summary.removed	+= newLinesB;
			}
			else
			{
				summary.added	+= newLinesB;
				summary.removed	+= newLinesA;
			}
		}
		else if (bd.a.len())
		{
			cmpInfo.a.range.s = bd.a.s;
			cmpInfo.a.range.e = bd.a.e;
			markSection(cmpInfo.a, bi, bd.a.s, options);

			const intptr_t movedLines = cmpInfo.a.movedRanges[bi].totalLinesCount();

			summary.diffLines	+= bd.a.len() - movedLines;
			summary.moved		+= movedLines;

			if (cmpInfo.a.diffMask == MARKER_MASK_ADDED)
				summary.added += bd.a.len() - movedLines;
			else
				summary.removed += bd.a.len() - movedLines;

			alignIdxA = bd.a.e;
		}
		else
		{
			cmpInfo.b.range.s = bd.b.s;
			cmpInfo.b.range.e = bd.b.e;
			markSection(cmpInfo.b, bi, bd.b.s, options);

			const intptr_t movedLines = cmpInfo.b.movedRanges[bi].totalLinesCount();

			summary.diffLines	+= bd.b.len() - movedLines;
			summary.moved		+= movedLines;

			if (cmpInfo.b.diffMask == MARKER_MASK_ADDED)
				summary.added += bd.b.len() - movedLines;
			else
				summary.removed += bd.b.len() - movedLines;

			alignIdxB = bd.b.e;
		}

		if (!progress->Advance())
			return false;
	}

	alignPair.main.diffMask	= 0;
	alignPair.main.line		= cmpInfo.a.getDocLine(cmpInfo.blockDiffs.back().a.e - 1) + 1;

	alignPair.sub.diffMask	= 0;
	alignPair.sub.line		= cmpInfo.b.getDocLine(cmpInfo.blockDiffs.back().b.e - 1) + 1;

	summary.alignmentInfo.emplace_back(alignPair);

	if (blockDiffsSize)
		summary.match += cmpInfo.a.lines.size() - cmpInfo.blockDiffs.back().a.e;

	summary.moved /= 2;

	if (!progress->NextPhase())
		return false;

	return true;
}


// Needed to format patch generation data
// std::vector<diff_section_t> toDiffSections(const CompareInfo& cmpInfo, const CompareOptions& options)
// {
	// std::vector<diff_section_t> diffSecs;

	// diffSecs.reserve(cmpInfo.blockDiffs.size());

	// intptr_t lineB		= 0;
	// intptr_t docLine1	= options.selectionCompare ? options.selections[0].first : 0;
	// intptr_t docLine2	= options.selectionCompare ? options.selections[1].first : 0;

	// for (const auto& bd : cmpInfo.blockDiffs)
	// {
		// if (bd.type == diff_type::DIFF_MATCH)
		// {
			// lineB += bd.len;

			// const intptr_t endLine1 = cmpInfo.a.getLine(bd.off + bd.len);
			// const intptr_t endLine2 = cmpInfo.b.getLine(lineB);

			// diffSecs.emplace_back(DiffType::MATCH, docLine1, endLine1 - docLine1, docLine2, endLine2 - docLine2);

			// docLine1 = endLine1;
			// docLine2 = endLine2;
		// }
		// else if (bd.type == diff_type::DIFF_IN_1)
		// {
			// const intptr_t endLine1 = cmpInfo.a.getLine(bd.off + bd.len - 1) + 1;

			// docLine1 = cmpInfo.a.getLine(bd.off);

			// diffSecs.emplace_back(DiffType::IN_1, docLine1, endLine1 - docLine1, docLine2, 0);

			// docLine1 = endLine1;
		// }
		// else // bd.type == diff_type::DIFF_IN_2
		// {
			// lineB = bd.off + bd.len;

			// const intptr_t endLine2 = cmpInfo.b.getLine(lineB - 1) + 1;

			// docLine2 = cmpInfo.b.getLine(bd.off);

			// diffSecs.emplace_back(DiffType::IN_2, docLine1, 0, docLine2, endLine2 - docLine2);

			// docLine2 = endLine2;
		// }
	// }

	// return diffSecs;
// }


CompareResult runCompare(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.clear();

	CompareInfo cmpInfo;

	if (options.selectionCompare)
	{
		cmpInfo.a.range.s = options.selections[cmpInfo.a.view].first;
		cmpInfo.a.range.e = options.selections[cmpInfo.a.view].second + 1;

		cmpInfo.b.range.s = options.selections[cmpInfo.b.view].first;
		cmpInfo.b.range.e = options.selections[cmpInfo.b.view].second + 1;
	}

	cmpInfo.a.diffMask = (options.newFileViewId == cmpInfo.a.view) ? MARKER_MASK_ADDED : MARKER_MASK_REMOVED;
	cmpInfo.b.diffMask = (options.newFileViewId == cmpInfo.a.view) ? MARKER_MASK_REMOVED : MARKER_MASK_ADDED;

	LOGD_GET_TIME;

	getLines(cmpInfo.a, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(cmpInfo.b, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	cmpInfo.blockDiffs = DiffCalc<Line>(cmpInfo.a.lines, cmpInfo.b.lines,
		std::bind(&ProgressDlg::IsCancelled, progress))(
			options.ignoreAllSpaces || options.ignoreChangedSpaces, true, options.syncPoints);

	if (progress->IsCancelled())
		return CompareResult::COMPARE_CANCELLED;

	LOGD_GET_TIME;
	PRINT_DIFFS("COMPARE START - LINE DIFFS", cmpInfo.blockDiffs);

	if (cmpInfo.blockDiffs.empty())
		return CompareResult::COMPARE_MATCH;

	cmpInfo.a.changedLines.resize(cmpInfo.blockDiffs.size());
	cmpInfo.b.changedLines.resize(cmpInfo.blockDiffs.size());
	cmpInfo.a.movedRanges.resize(cmpInfo.blockDiffs.size());
	cmpInfo.b.movedRanges.resize(cmpInfo.blockDiffs.size());

	findUniqueLines(cmpInfo);

	if (options.detectMoves)
		findMoves(cmpInfo);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	if (options.detectSubBlockDiffs)
		findSubBlockDiffs(cmpInfo, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	// Make sure we have at least one line in each view so the functions' logic below works properly
	if (cmpInfo.a.lines.empty())
		cmpInfo.a.lines.emplace_back(0, cHashSeed);
	if (cmpInfo.b.lines.empty())
		cmpInfo.b.lines.emplace_back(0, cHashSeed);

	// Needed for patch generation
	// summary.diff1view		= cmpInfo.a.view;
	// summary.diffSections	= toDiffSections(cmpInfo, options);

	if (!markAllDiffs(cmpInfo, options, summary))
		return CompareResult::COMPARE_CANCELLED;

	return CompareResult::COMPARE_MISMATCH;
}


CompareResult runFindUnique(const CompareOptions& options, CompareSummary& summary)
{
	progress_ptr& progress = ProgressDlg::Get();

	summary.clear();

	DocCmpInfo a(MAIN_VIEW, &diff_info::a);
	DocCmpInfo b(SUB_VIEW, &diff_info::b);

	if (options.selectionCompare)
	{
		a.range.s = options.selections[a.view].first;
		a.range.e = options.selections[a.view].second + 1;

		b.range.s = options.selections[b.view].first;
		b.range.e = options.selections[b.view].second + 1;
	}

	if (options.newFileViewId == a.view)
	{
		a.diffMask = MARKER_MASK_ADDED;
		b.diffMask = MARKER_MASK_REMOVED;
	}
	else
	{
		a.diffMask = MARKER_MASK_REMOVED;
		b.diffMask = MARKER_MASK_ADDED;
	}

	getLines(a, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	getLines(b, options);

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<Line::HashType, std::vector<intptr_t>> aUniqueLines;

	for (const auto& line : a.lines)
	{
		auto insertPair = aUniqueLines.emplace(line.hash, std::vector<intptr_t>{line.num});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.num);
	}

	a.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	std::unordered_map<Line::HashType, std::vector<intptr_t>> bUniqueLines;

	for (const auto& line : b.lines)
	{
		auto insertPair = bUniqueLines.emplace(line.hash, std::vector<intptr_t>{line.num});
		if (!insertPair.second)
			insertPair.first->second.emplace_back(line.num);
	}

	b.lines.clear();

	if (!progress->NextPhase())
		return CompareResult::COMPARE_CANCELLED;

	clearWindow(MAIN_VIEW, false);
	clearWindow(SUB_VIEW, false);

	intptr_t aUniqueLinesCount = 0;

	for (const auto& uniqueLine : aUniqueLines)
	{
		auto bItr = bUniqueLines.find(uniqueLine.first);

		if (bItr != bUniqueLines.end())
		{
			bUniqueLines.erase(bItr);
			++summary.match;
		}
		else
		{
			for (const auto& line : uniqueLine.second)
			{
				markLine(a.view, line, a.diffMask);
				++aUniqueLinesCount;
			}
		}
	}

	if (aUniqueLinesCount == 0 && bUniqueLines.empty())
		return CompareResult::COMPARE_MATCH;

	if (a.diffMask == MARKER_MASK_ADDED)
		summary.added = aUniqueLinesCount;
	else
		summary.removed = aUniqueLinesCount;

	for (const auto& uniqueLine : bUniqueLines)
	{
		for (const auto& line : uniqueLine.second)
			markLine(b.view, line, b.diffMask);

		if (b.diffMask == MARKER_MASK_ADDED)
			summary.added += uniqueLine.second.size();
		else
			summary.removed += uniqueLine.second.size();
	}

	summary.diffLines = summary.added + summary.removed;

	AlignmentPair align;
	align.main.line	= a.range.s;
	align.sub.line	= b.range.s;

	summary.alignmentInfo.push_back(align);

	return CompareResult::COMPARE_MISMATCH;
}

}


CompareResult compareViews(const CompareOptions& options, const wchar_t* progressInfo, CompareSummary& summary)
{
	CompareResult result = CompareResult::COMPARE_ERROR;

	if (!progressInfo || !ProgressDlg::Open(progressInfo))
		return CompareResult::COMPARE_ERROR;

	clearChangedIndicatorFull(MAIN_VIEW);
	clearChangedIndicatorFull(SUB_VIEW);

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

		std::string msg = "Exception occurred: ";
		msg += e.what();

		::MessageBoxA(nppData._nppHandle, msg.c_str(), "ComparePlus", MB_OK | MB_ICONWARNING);
	}
	catch (...)
	{
		ProgressDlg::Close();

		::MessageBoxA(nppData._nppHandle, "Unknown exception occurred.", "ComparePlus", MB_OK | MB_ICONWARNING);
	}

	return result;
}
