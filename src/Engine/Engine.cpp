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
#include <span>
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
	Word(intptr_t idx, intptr_t p, intptr_t l, charType t, uint64_t h = cHashSeed) :
		hash_type<uint64_t>(h), lineIdx(idx),  pos(p), len(l), type(t) {}

	intptr_t lineIdx;
	intptr_t pos;
	intptr_t len;
	charType type;
};


// Compared element 'Char'
// Use directly the character as hash
struct Char : public hash_type<wchar_t>
{
	static const intptr_t len = 1;

	Char(wchar_t c, intptr_t p) : hash_type<wchar_t>(c), pos(p) {}

	intptr_t pos;
};


struct changed_range_t : public range_t
{
	changed_range_t() : range_t(), moved_to {-1} {}
	changed_range_t(const range_t& rhs) : range_t(rhs), moved_to {-1} {}
	changed_range_t(intptr_t start, intptr_t end) : range_t(start, end), moved_to {-1} {}

	intptr_t moved_to;
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

	std::vector<std::vector<ChangedLine>>	changedLines;	// Changed lines per block diff (sub-block compare)
	std::vector<MovedRanges>				movedRanges;	// Moved lines ranges per block diff

	inline const range_t& diffRange(const diff_info& di) const
	{
		return (di.*diffPtr);
	}

	inline const Line& getLine(intptr_t idx) const
	{
		assert(idx >= 0 && static_cast<size_t>(idx) < lines.size());

		return lines[idx];
	}

	inline const Line& getLine(const diff_info& di, intptr_t off = 0) const
	{
		assert(off >= 0 && off < diffRange(di).len() && static_cast<size_t>(diffRange(di).s + off) < lines.size());

		return lines[diffRange(di).s + off];
	}

	inline intptr_t getDocLine(intptr_t idx) const
	{
		assert(idx >= 0);

		if (static_cast<size_t>(idx) >= lines.size())
			return lines.back().num;

		return lines[idx].num;
	}

	inline intptr_t getDocLine(const diff_info& di, intptr_t off = 0) const
	{
		assert(off >= 0 && off < diffRange(di).len());

		if (static_cast<size_t>(diffRange(di).s + off) >= lines.size())
			return lines.back().num;

		return lines[diffRange(di).s + off].num;
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
	constexpr int monitorCancelEveryXLine = 10000;

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

	Word word {lineIdx, pos, 1, currentWordType};

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
			word.type = currentWordType;

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
		words = getRegexIgnoreLineWords(wLine, lineIdx, options);
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

		getSectionRangeWords(words, wLine, lineIdx, pos, endPos, options);
	}

	// In case of UTF-16 or UTF-32 find words byte positions and lengths because Scintilla uses those
	if (wLen != len)
		recalculateWordPos(codepage, words, wLine);

	return words;
}


std::pair<std::vector<Word>, std::unordered_map<intptr_t, range_t>> getLinesRangeWords(const DocCmpInfo& doc,
	const range_t& range, intptr_t diffIdx, const CompareOptions& options)
{
	const int codepage = getCodepage(doc.view);

	std::vector<Word> words;
	std::unordered_map<intptr_t, range_t> lineWordsRange;

	for (intptr_t l = range.s; l < range.e; ++l)
	{
		intptr_t lIdx = l - range.s;

		if (doc.movedRanges[diffIdx].getNextUnmoved(lIdx))
		{
			l = lIdx + range.s;
			if (l >= range.e)
				break;
		}

		std::vector<Word> lineWords = getLineWords(doc.view, codepage, doc.getDocLine(l), options, lIdx);

		if (!lineWords.empty())
		{
			const intptr_t rangeS = static_cast<intptr_t>(words.size());
			words.insert(words.end(), lineWords.begin(), lineWords.end());
			lineWordsRange.emplace(lIdx, range_t(rangeS, static_cast<intptr_t>(words.size())));
		}
	}

	return std::make_pair(words, lineWordsRange);
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
					size_t i = chars[l].size() - 1;

					for (; i >= 0 && (chars[l][i] == L' ' || chars[l][i] == L'\t'); --i);

					if (++i != chars[l].size())
						chars[l].erase(chars[l].begin() + i, chars[l].end());

					auto itr = chars[l].begin();

					for (; itr != chars[l].end() && (itr->hash == L' ' || itr->hash == L'\t'); ++itr);

					if (itr != chars[l].begin())
						chars[l].erase(chars[l].begin(), itr);
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
		intptr_t diffIdxA	{0};
		intptr_t offA		{0};
		intptr_t diffIdxB	{0};
		intptr_t offB		{0};
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


// Elem type must have == operator
template <typename Elem>
void findMovedRanges(const std::vector<Elem>& dataA, const std::vector<Elem>& dataB,
	std::vector<changed_range_t>& rangesA, std::vector<changed_range_t>& rangesB)
{
	std::unordered_set<intptr_t> nonUniqueRangeA;
	std::unordered_set<intptr_t> nonUniqueRangeB;

	for (auto ra = rangesA.begin(); ra != rangesA.end(); ra++)
	{
		if (nonUniqueRangeA.find(ra->s) != nonUniqueRangeA.end())
			continue;

		bool sameRangeFound = false;

		for (auto r = ra + 1; r != rangesA.end(); r++)
		{
			if (r->len() == ra->len() && std::equal(&dataA[r->s], &dataA[r->e], &dataA[ra->s]))
			{
				nonUniqueRangeA.emplace(r->s);
				sameRangeFound = true;
			}
		}

		if (sameRangeFound)
			continue;

		auto rbMatch = rangesB.end();

		for (auto rb = rangesB.begin(); rb != rangesB.end(); rb++)
		{
			if (rb->len() == ra->len() && rb->moved_to < 0 && nonUniqueRangeB.find(rb->s) == nonUniqueRangeB.end() &&
				std::equal(&dataB[rb->s], &dataB[rb->e], &dataA[ra->s]))
			{
				if (rbMatch == rangesB.end())
				{
					rbMatch = rb;
				}
				else
				{
					nonUniqueRangeB.emplace(rb->s);
					nonUniqueRangeB.emplace(rbMatch->s);
					rbMatch = rangesB.end();
					break;
				}
			}
		}

		if (rbMatch != rangesB.end())
		{
			ra->moved_to = rbMatch->s;
			rbMatch->moved_to = ra->s;
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

		std::vector<Char> secA;
		std::vector<Char> secB;

		std::vector<changed_range_t> changesA;
		std::vector<changed_range_t> changesB;

		// First use word granularity (find matching words) for better precision
		const auto lineDiffs = DiffCalc<Word>(lineWordsA, lineWordsB)(DiffAlg::MIXED, true, true);

		PRINT_DIFFS("WORD DIFFS", lineDiffs);

		a.changedLines[diffIdx].emplace_back(lm.second);
		b.changedLines[diffIdx].emplace_back(lm.first);

		auto& changedLineA = a.changedLines[diffIdx].back();
		auto& changedLineB = b.changedLines[diffIdx].back();

		const intptr_t linePosA = getLineStart(a.view, lineA);
		const intptr_t linePosB = getLineStart(b.view, lineB);

		for (const auto& ld : lineDiffs)
		{
			// Resolve words mismatched pairs to find possible sub-word similarities
			if (ld.is_replacement())
			{
				intptr_t offA = lineWordsA[ld.a.s].pos;
				intptr_t endA = lineWordsA[ld.a.e - 1].pos + lineWordsA[ld.a.e - 1].len;

				intptr_t offB = lineWordsB[ld.b.s].pos;
				intptr_t endB = lineWordsB[ld.b.e - 1].pos + lineWordsB[ld.b.e - 1].len;

				secA = getSectionChars(a.view, linePosA + offA, linePosA + endA, options);
				secB = getSectionChars(b.view, linePosB + offB, linePosB + endB, options);

				if (options.detectCharDiffs)
				{
					LOGD(LOG_ALGO, "Compare Sections " +
							std::to_string(offA + 1) + " to " + std::to_string(endA + 1) + " and " +
							std::to_string(offB + 1) + " to " + std::to_string(endB + 1) + "\n");

					// Compare changed words
					const auto sectionDiffs = DiffCalc<Char>(secA, secB)(DiffAlg::MYERS, true, true);

					PRINT_DIFFS("CHAR DIFFS", sectionDiffs);

					for (const auto& sd : sectionDiffs)
					{
						if (sd.a.len())
						{
							if (options.detectSubLineMoves)
								changesA.emplace_back(sd.a);

							changedLineA.changes.emplace_back(
									offA + secA[sd.a.s].pos, offA + secA[sd.a.e - 1].pos + 1);
						}
						if (sd.b.len())
						{
							if (options.detectSubLineMoves)
								changesB.emplace_back(sd.b);

							changedLineB.changes.emplace_back(
									offB + secB[sd.b.s].pos, offB + secB[sd.b.e - 1].pos + 1);
						}
					}
				}
				// Always match non-alphabetical characters in the beginning and at the end of changed sections
				else
				{
					if (options.detectSubLineMoves)
					{
						changesA.emplace_back(ld.a);
						changesB.emplace_back(ld.b);
					}

					matchBeginEnd(changedLineA, changedLineB, secA, secB, offA, offB, endA, endB,
								[](const wchar_t ch) { return (getCharTypeW(ch) != charType::ALPHANUMCHAR); });
				}
			}
			else if (ld.a.len())
			{
				if (options.detectSubLineMoves)
					changesA.emplace_back(ld.a);

				changedLineA.changes.emplace_back(
						lineWordsA[ld.a.s].pos, lineWordsA[ld.a.e - 1].pos + lineWordsA[ld.a.e - 1].len);
			}
			else
			{
				if (options.detectSubLineMoves)
					changesB.emplace_back(ld.b);

				changedLineB.changes.emplace_back(
						lineWordsB[ld.b.s].pos, lineWordsB[ld.b.e - 1].pos + lineWordsB[ld.b.e - 1].len);
			}
		}

		if (options.detectSubLineMoves)
		{
			if (!changedLineA.changes.empty() && !changedLineB.changes.empty())
			{
				if (options.detectCharDiffs)
					findMovedRanges(secA, secB, changesA, changesB);
				else
					findMovedRanges(lineWordsA, lineWordsB, changesA, changesB);

				intptr_t changesCount = static_cast<intptr_t>(changedLineA.changes.size());

				for (intptr_t i = 0; i < changesCount; ++i)
					changedLineA.changes[i].moved_to = changesA[i].moved_to;

				changesCount = static_cast<intptr_t>(changedLineB.changes.size());

				for (intptr_t i = 0; i < changesCount; ++i)
					changedLineB.changes[i].moved_to = changesB[i].moved_to;
			}
		}
	}
}


inline std::span<Word> getLineSpan(
	std::pair<std::vector<Word>, std::unordered_map<intptr_t, range_t>>& range, intptr_t idx)
{
	assert(idx >= 0 && static_cast<size_t>(idx) < range.first.size());

	auto itr = range.second.find(range.first[idx].lineIdx);
	assert(itr != range.second.end());

	return std::span<Word>(range.first.begin() + itr->second.s, static_cast<size_t>(itr->second.len()));
}


float findResemblance(const std::span<Word> sA, const std::span<Word> sB)
{
	intptr_t totalLen = 0;
	intptr_t matchLen = 0;

	for (const auto& w : sA)
		totalLen += w.len;

	for (const auto& w : sB)
		totalLen += w.len;

	if (!totalLen)
		return 0;

	progress_ptr& progress = ProgressDlg::Get();

	const auto wordDiffs = DiffCalc<Word>(sA, sB, std::bind(&ProgressDlg::IsCancelled, progress))(DiffAlg::MIXED);

	if (progress->IsCancelled())
		return 0;

	const intptr_t wordDiffsSize = static_cast<intptr_t>(wordDiffs.size());

	if (!wordDiffsSize)
		return 100.0;

	for (intptr_t n = 0; n < wordDiffs[0].a.s; ++n)
		matchLen += sA[n].len;

	for (intptr_t n = wordDiffs[wordDiffsSize - 1].a.e; static_cast<size_t>(n) < sA.size(); ++n)
		matchLen += sA[n].len;

	for (intptr_t i = 1; i < wordDiffsSize; ++i)
	{
		for (intptr_t n = wordDiffs[i - 1].a.e; n < wordDiffs[i].a.s; ++n)
			matchLen += sA[n].len;
	}

	return (static_cast<float>(matchLen * 2 * 100)) / totalLen;
}


float findResemblance(const std::vector<Char> lA, const std::vector<Char> lB, int changedResemblPercent)
{
	const intptr_t minSize = std::min(lA.size(), lB.size());
	const intptr_t maxSize = std::max(lA.size(), lB.size());

	if ((static_cast<float>(minSize * 100) / maxSize) < changedResemblPercent)
		return 0;

	progress_ptr& progress = ProgressDlg::Get();

	const auto charDiffs = DiffCalc<Char>(lA, lB, std::bind(&ProgressDlg::IsCancelled, progress))(DiffAlg::MYERS);

	if (progress->IsCancelled())
		return 0;

	if (charDiffs.empty())
		return 100.0;

	const intptr_t totalLen = lA.size() + lB.size();
	intptr_t matchLen = totalLen;

	for (const auto& cd : charDiffs)
		matchLen -= (cd.a.len() + cd.b.len());

	const float conv = (static_cast<float>(matchLen * 100)) / totalLen;

	return conv < changedResemblPercent ? 0 : conv;
}


bool findChangesLineByLine(CompareInfo& cmpInfo, intptr_t diffIdx, const CompareOptions& options)
{
	const std::vector<std::vector<Char>> linesA =
			getLinesChars(cmpInfo.a, cmpInfo.blockDiffs[diffIdx], diffIdx, options);
	const std::vector<std::vector<Char>> linesB =
			getLinesChars(cmpInfo.b, cmpInfo.blockDiffs[diffIdx], diffIdx, options);

	if (linesA.empty() || linesB.empty())
		return true;

	const intptr_t linesCountA = static_cast<intptr_t>(linesA.size());
	const intptr_t linesCountB = static_cast<intptr_t>(linesB.size());

	std::map<intptr_t, intptr_t> changedLines; // lineB -> lineA

	std::unordered_map<uint64_t, float> linesResemblance;

	float bestResemblance = 0;
	intptr_t bal = 0;
	intptr_t bbl = 0;

	for (intptr_t al = 0; al < linesCountA; ++al)
	{
		for (intptr_t bl = 0; bl < linesCountB; ++bl)
		{
			const uint64_t lines = (static_cast<uint64_t>(al << 31) | static_cast<uint64_t>(bl));
			const float resemblance = findResemblance(linesA[al], linesB[bl], options.changedResemblPercent);
			linesResemblance.emplace(lines, resemblance);

			if (resemblance > bestResemblance)
			{
				bestResemblance = resemblance;
				bal = al;
				bbl = bl;
			}
			else if (resemblance == bestResemblance && (bal + bbl > al + bl))
			{
				bal = al;
				bbl = bl;
			}
		}
	}

	if (!bestResemblance)
		return true;

	std::vector<range_t> stack;

	changedLines.emplace(bbl, bal);

	if (bal > 0 && bbl > 0)
	{
		stack.emplace_back(0, bal);
		stack.emplace_back(0, bbl);
	}

	if (bal + 1 < linesCountA && bbl + 1 < linesCountB)
	{
		stack.emplace_back(bal + 1, linesCountA);
		stack.emplace_back(bbl + 1, linesCountB);
	}

	while (!stack.empty())
	{
		range_t rangeB = stack.back();
		stack.pop_back();
		range_t rangeA = stack.back();
		stack.pop_back();

		bestResemblance = 0;
		bal = 0;
		bbl = 0;

		for (intptr_t al = rangeA.s; al < rangeA.e; ++al)
		{
			for (intptr_t bl = rangeB.s; bl < rangeB.e; ++bl)
			{
				const uint64_t lines = (static_cast<uint64_t>(al << 31) | static_cast<uint64_t>(bl));
				auto lItr = linesResemblance.find(lines);
				assert(lItr != linesResemblance.end());

				if (lItr->second > bestResemblance)
				{
					bestResemblance = lItr->second;
					bal = al;
					bbl = bl;
				}
				else if (lItr->second == bestResemblance && (bal + bbl > al + bl))
				{
					bal = al;
					bbl = bl;
				}
			}
		}

		if (!bestResemblance)
			continue;

		changedLines.emplace(bbl, bal);

		if (bal > rangeA.s && bbl > rangeB.s)
		{
			stack.emplace_back(rangeA.s, bal);
			stack.emplace_back(rangeB.s, bbl);
		}

		if (bal + 1 < rangeA.e && bbl + 1 < rangeB.e)
		{
			stack.emplace_back(bal + 1, rangeA.e);
			stack.emplace_back(bbl + 1, rangeB.e);
		}
	}

	compareLines(cmpInfo, diffIdx, changedLines, options);

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


bool findChangesByWords(CompareInfo& cmpInfo, intptr_t diffIdx, const CompareOptions& options)
{
	struct MatchingWord // line idx -> word idx
	{
		std::map<intptr_t, intptr_t> a;
		std::map<intptr_t, intptr_t> b;
	};

	std::pair<std::vector<Word>, std::unordered_map<intptr_t, range_t>> wordsRangeA =
			getLinesRangeWords(cmpInfo.a, cmpInfo.blockDiffs[diffIdx].a, diffIdx, options);
	std::pair<std::vector<Word>, std::unordered_map<intptr_t, range_t>> wordsRangeB =
			getLinesRangeWords(cmpInfo.b, cmpInfo.blockDiffs[diffIdx].b, diffIdx, options);

	std::vector<Word>& wordsA = wordsRangeA.first;
	std::vector<Word>& wordsB = wordsRangeB.first;
	std::unordered_map<intptr_t, range_t>& lineWordsRangeA = wordsRangeA.second;
	std::unordered_map<intptr_t, range_t>& lineWordsRangeB = wordsRangeB.second;

	if (wordsA.empty() || wordsB.empty())
		return true;

	std::map<intptr_t, intptr_t> changedLines; // lineB -> lineA

	std::unordered_map<uint64_t, float> linesResemblance;
	std::vector<range_t> stack;

	stack.emplace_back(0, static_cast<intptr_t>(wordsA.size()));
	stack.emplace_back(0, static_cast<intptr_t>(wordsB.size()));

	while (!stack.empty())
	{
		range_t rangeB = stack.back();
		stack.pop_back();
		range_t rangeA = stack.back();
		stack.pop_back();

		if (wordsA[rangeA.s].lineIdx == wordsA[rangeA.e - 1].lineIdx &&
			wordsB[rangeB.s].lineIdx == wordsB[rangeB.e - 1].lineIdx)
		{
			float resemblance;
			const uint64_t lines = (
							static_cast<uint64_t>(wordsA[rangeA.s].lineIdx) << 31) |
							static_cast<uint64_t>(wordsB[rangeB.s].lineIdx);
			auto lItr = linesResemblance.find(lines);

			if (lItr != linesResemblance.end())
			{
				resemblance = lItr->second;
			}
			else
			{
				resemblance = findResemblance(getLineSpan(wordsRangeA, rangeA.s), getLineSpan(wordsRangeB, rangeB.s));
				linesResemblance.emplace(lines, resemblance);
			}

			if (resemblance >= options.changedResemblPercent)
				changedLines.emplace(wordsB[rangeB.s].lineIdx, wordsA[rangeA.s].lineIdx);

			continue;
		}

		MatchingWord* bestMatchingWord = nullptr;

		for (int run = 2; run; --run)
		{
			std::unordered_map<Word::HashType, MatchingWord> wordMatchMap;

			if (run == 2)
			{
				for (intptr_t i = rangeA.s; i < rangeA.e; ++i)
					if (wordsA[i].type == charType::ALPHANUMCHAR)
						wordMatchMap.emplace(
							wordsA[i].hash, MatchingWord{}).first->second.a.emplace(wordsA[i].lineIdx, i);

				for (intptr_t i = rangeB.s; i < rangeB.e; ++i)
					if (wordsB[i].type == charType::ALPHANUMCHAR)
						wordMatchMap.emplace(
							wordsB[i].hash, MatchingWord{}).first->second.b.emplace(wordsB[i].lineIdx, i);
			}
			else
			{
				for (intptr_t i = rangeA.s; i < rangeA.e; ++i)
					if (wordsA[i].type != charType::ALPHANUMCHAR)
						wordMatchMap.emplace(
							wordsA[i].hash, MatchingWord{}).first->second.a.emplace(wordsA[i].lineIdx, i);

				for (intptr_t i = rangeB.s; i < rangeB.e; ++i)
					if (wordsB[i].type != charType::ALPHANUMCHAR)
						wordMatchMap.emplace(
							wordsB[i].hash, MatchingWord{}).first->second.b.emplace(wordsB[i].lineIdx, i);
			}

			if (wordMatchMap.empty())
				continue;

			size_t minOccurrence = SIZE_MAX;

			bestMatchingWord = &(wordMatchMap.begin()->second);

			// It might be beneficial to make convergence checks for each a and b lines of each m
			// to find the best word matching line
			for (auto& m : wordMatchMap)
			{
				if (m.second.a.empty() || m.second.b.empty())
					continue;

				const size_t occurrence = m.second.a.size() + m.second.b.size();

				if (occurrence <= minOccurrence)
				{
					if (occurrence < minOccurrence || (
						m.second.a.begin()->first + m.second.b.begin()->first <
						bestMatchingWord->a.begin()->first + bestMatchingWord->b.begin()->first))
					{
						float resemblance;
						const uint64_t lines = (
										static_cast<uint64_t>(m.second.a.begin()->first) << 31) |
										static_cast<uint64_t>(m.second.b.begin()->first);
						auto lItr = linesResemblance.find(lines);

						if (lItr != linesResemblance.end())
						{
							resemblance = lItr->second;
						}
						else
						{
							resemblance = findResemblance(
									getLineSpan(wordsRangeA, m.second.a.begin()->second),
									getLineSpan(wordsRangeB, m.second.b.begin()->second));
							linesResemblance.emplace(lines, resemblance);
						}

						if (resemblance >= options.changedResemblPercent)
						{
							bestMatchingWord = &m.second;
							minOccurrence = occurrence;
						}
					}
				}
			}

			if (minOccurrence != SIZE_MAX)
				break;

			bestMatchingWord = nullptr;
		}

		if (bestMatchingWord == nullptr)
			continue;

		const intptr_t al = bestMatchingWord->a.begin()->first;
		const intptr_t bl = bestMatchingWord->b.begin()->first;

		auto lrA = lineWordsRangeA.find(al);
		assert(lrA != lineWordsRangeA.end());

		auto lrB = lineWordsRangeB.find(bl);
		assert(lrB != lineWordsRangeB.end());

		changedLines.emplace(bl, al);

		if (lrA->second.s > rangeA.s && lrB->second.s > rangeB.s)
		{
			stack.emplace_back(rangeA.s, lrA->second.s);
			stack.emplace_back(rangeB.s, lrB->second.s);
		}

		if (lrA->second.e < rangeA.e && lrB->second.e < rangeB.e)
		{
			stack.emplace_back(lrA->second.e, rangeA.e);
			stack.emplace_back(lrB->second.e, rangeB.e);
		}
	}

	compareLines(cmpInfo, diffIdx, changedLines, options);

	return true;
}


void findSubBlockDiffs(CompareInfo& cmpInfo, const CompareOptions& options)
{
	progress_ptr& progress = ProgressDlg::Get();

	std::vector<intptr_t> changedBlockIdx;

	const intptr_t blockDiffsSize = static_cast<intptr_t>(cmpInfo.blockDiffs.size());

	// Get changed blocks to sub-compare
	for (intptr_t i = 0; i < blockDiffsSize; ++i)
	{
		if (cmpInfo.blockDiffs[i].is_replacement())
			changedBlockIdx.emplace_back(i);
	}

	if (changedBlockIdx.empty())
		return;

	progress->SetMaxCount(static_cast<intptr_t>(changedBlockIdx.size()));

// #ifdef MULTITHREAD // Do multithreaded block compares

	// const int threadsCount = std::thread::hardware_concurrency() - 2;

	// if (threadsCount > 0)
	// {
		// LOGD(LOG_ALL, "Changes detection running on " + std::to_string(threadsCount + 1) + " threads\n");

		// std::atomic<size_t> blockIdx {0};

		// auto threadFn =
			// [&]()
			// {
				// for (size_t i = blockIdx++; i < changedBlockIdx.size(); i = blockIdx++)
				// {
					// if (!findChangesLineByLine(cmpInfo, changedBlockIdx[i], options))
						// return;
				// }
			// };

		// std::vector<std::thread> threads(threadsCount);

		// for (auto& th : threads)
			// th = std::thread(threadFn);

		// threadFn();

		// for (auto& th : threads)
			// th.join();
	// }
	// else
	// {
		// LOGD(LOG_ALL, "Changes detection running on 1 thread\n");

		// for (intptr_t i : changedBlockIdx)
		// {
			// if (!findChangesLineByLine(cmpInfo, i, options))
				// break;
		// }
	// }

// #else // Do block compares in single thread

	for (intptr_t i : changedBlockIdx)
	{
		if (options.detectCharDiffs && options.ignoreAllSpaces)
		{
			if (!findChangesLineByLine(cmpInfo, i, options))
				break;
		}
		else
		{
			if (!findChangesByWords(cmpInfo, i, options))
				break;
		}

		progress->Advance();
	}

// #endif // MULTITHREAD
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
						change.moved_to < 0 ? color : Settings.colors().moved_part);

	markLine(cmpInfo.a.view, line, cmpInfo.a.nonUniqueDocLines.find(line) == cmpInfo.a.nonUniqueDocLines.end() ?
			MARKER_MASK_CHANGED : MARKER_MASK_CHANGED_LOCAL);

	line = cmpInfo.b.getDocLine(cmpInfo.blockDiffs[bi], changedLineB.idx);
	linePos = getLineStart(cmpInfo.b.view, line);
	color = (cmpInfo.b.diffMask == MARKER_MASK_ADDED) ?
			Settings.colors().added_part : Settings.colors().removed_part;

	for (const auto& change : changedLineB.changes)
		markTextAsChanged(cmpInfo.b.view, linePos + change.s, change.len(),
						change.moved_to < 0 ? color : Settings.colors().moved_part);

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

	if (!cmpInfo.a.lines.empty() && !cmpInfo.b.lines.empty())
	{
		alignPair.main.diffMask	= 0;
		alignPair.main.line		= cmpInfo.blockDiffs.back().a.e > 0 ?
			cmpInfo.a.getDocLine(cmpInfo.blockDiffs.back().a.e - 1) + 1 :
			cmpInfo.a.getDocLine(cmpInfo.blockDiffs.back().a.e);

		alignPair.sub.diffMask	= 0;
		alignPair.sub.line		= cmpInfo.blockDiffs.back().b.e > 0 ?
			cmpInfo.b.getDocLine(cmpInfo.blockDiffs.back().b.e - 1) + 1 :
			cmpInfo.b.getDocLine(cmpInfo.blockDiffs.back().b.e);

		summary.alignmentInfo.emplace_back(alignPair);
	}

	if (blockDiffsSize)
		summary.match += cmpInfo.a.lines.size() - cmpInfo.blockDiffs.back().a.e;

	summary.moved /= 2;

	if (!progress->NextPhase())
		return false;

	return true;
}


// Needed to format patch generation data
void toDocLineDiffSections(CompareInfo& cmpInfo)
{
	DocCmpInfo&	a = cmpInfo.a;
	DocCmpInfo&	b = cmpInfo.b;

	for (auto& bd : cmpInfo.blockDiffs)
	{
		bd.a.s = a.getDocLine(bd.a.s);
		bd.a.e = a.getDocLine(bd.a.e);
		bd.b.s = b.getDocLine(bd.b.s);
		bd.b.e = b.getDocLine(bd.b.e);
	}
}


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
		std::bind(&ProgressDlg::IsCancelled, progress))(DiffAlg::MIXED,
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

	if (!markAllDiffs(cmpInfo, options, summary))
		return CompareResult::COMPARE_CANCELLED;

	toDocLineDiffSections(cmpInfo);

	// Needed for patch generation
	summary.aDiffView		= cmpInfo.a.view;
	summary.diffSections	= std::move(cmpInfo.blockDiffs);

	PRINT_DIFFS("Patch diff sections:", summary.diffSections);

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
