/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Font.h"

#include "win32def.h"

#include "GameData.h"
#include "Interface.h"
#include "Palette.h"
#include "Sprite2D.h"
#include "Video.h"

#include <sstream>

namespace GemRB {

Font::Font()
: resRefs(NULL), numResRefs(0), palette(NULL), maxHeight(0)
{
	name[0] = '\0';
}

Font::~Font(void)
{
	blank->release();
	SetPalette(NULL);
	free(resRefs);
}

bool Font::AddResRef(const ieResRef resref)
{
	if (resref) {
		resRefs = (ieResRef*)realloc(resRefs, sizeof(ieResRef) * ++numResRefs);
		strnlwrcpy( resRefs[numResRefs - 1], resref, sizeof(ieResRef)-1);
		return true;
	}
	return false;
}

bool Font::MatchesResRef(const ieResRef resref)
{
	for (int i=0; i < numResRefs; i++)
	{
		if (strnicmp( resref, resRefs[i], sizeof(ieResRef)-1) == 0){
			return true;
		}
	}
	return false;
}

Sprite2D* Font::RenderText(const String& string, const Size& size, size_t* numPrinted) const
{
	Size canvasSize = StringSize(string); // same as size(0, 0)
	// if the string is larger than the region shrink the canvas
	// except 0 means we should size to fit in that dimension
	if (size.w) {
		// potentially resize
		if (size.w < canvasSize.w) {
			// we need to resize horizontally which creates new lines
			ieWord trimmedArea = (canvasSize.w - size.w) * canvasSize.h;
			// this automatically becomes multiline, therefore use maxHeight
			ieWord lineArea = size.w * maxHeight;
			// round up
			ieWord numLines = 1 + ((trimmedArea - 1) / lineArea);
			if (!size.h) {
				// grow as much as needed vertically.
				canvasSize.h += numLines * maxHeight;
				// FIXME: there is a chance we didn't grow enough vertically...
				// we can't possibly know how lines will break ahead of time,
				// over a long enough paragraph we can overflow the canvas
				// the easiest solution would be to perform a realloc when it happens (and size.h == 0)
				// by calculating the additional size by the current substring
			} else if (size.h > canvasSize.h) {
				// grow by line increments until we hit the limit
				// round up, because even a partial line should be blitted (and clipped)
				ieWord maxLines = 1 + (((size.h - canvasSize.h) - 1) / maxHeight);
				if (maxLines < numLines) {
					numLines = maxLines;
				}
				canvasSize.h += numLines * maxHeight;
				// if the new canvas size is taller than size.h it will be dealt with later
			}
			canvasSize.w = size.w;
		}
		// else: we already fit in the designated area (horizontally). No need to resize.
	}
	if (size.h && size.h < canvasSize.h) {
		// we can't unbreak lines ("\n") so at best we can clip the text.
		canvasSize.h = size.h;
	}
	ieWord lineHeight = (maxHeight > canvasSize.h) ? canvasSize.h : maxHeight;

	ieDword ck = GetCharSprite(string[0])->GetColorKey();
	// we must calloc/memset because not all glyphs are equal height. set remainder to the color key
	ieByte* canvasPx = (ieByte*)calloc(canvasSize.w, canvasSize.h);
	ieByte* dest = canvasPx;
	if (ck != 0) {
		// start with transparent canvas
		memset(canvasPx, ck, canvasSize.w * canvasSize.h);
	}

	const Sprite2D* curGlyph = NULL;
	size_t len = string.length();
	size_t chrIdx = 0;
	int wCurrent = 0, hCurrent = 0, offset = 0;
	bool newline = false;
	for (; chrIdx < len; chrIdx++) {
		curGlyph = GetCharSprite(string[chrIdx]);
		assert(!curGlyph->BAM);
		if (newline) {
			hCurrent += lineHeight;
			if ((hCurrent + curGlyph->Height) > canvasSize.h) {
				// the next line would be clipped. we are done.
				// FIXME: we should support partial rows
				break;
			}
		}

		// copy the glyph to the canvas
		ieByte* src = (ieByte*)curGlyph->pixels;
		offset = (lineHeight - curGlyph->YPos) * canvasSize.w;
		assert(offset >= 0);
		dest = canvasPx + wCurrent + offset + (hCurrent * canvasSize.w);
		// FIXME: this can overflow due to glyphs that are only partially on the canvas
		for( int row = 0; row < curGlyph->Height; row++ ) {
			memcpy(dest, src, curGlyph->Width);
			dest += canvasSize.w;
			src += curGlyph->Width;
		}

		wCurrent += curGlyph->Width;
		if (wCurrent >= canvasSize.w) {
			// we've run off the edge of the region
			newline = true;
		}
	}
	// should have filled the canvas 100% <- NO! we probably have extra space on the last line
	//assert((dest - canvasPx) == (w * h));

	if (numPrinted) {
		*numPrinted = chrIdx;
	}
	Sprite2D* canvas = core->GetVideoDriver()->CreateSprite8(canvasSize.w, canvasSize.h,
															 canvasPx, palette, true, ck);
	// TODO: adjust canvas position based on alignment flags and rgn
	return canvas;
}

size_t Font::Print(Region rgn, const char* string,
				   Palette* hicolor, ieByte Alignment) const
{
	String* tmp = StringFromCString(string);
	size_t ret = Print(rgn, *tmp, hicolor, Alignment);
	delete tmp;
	return ret;
}

size_t Font::Print(Region rgn, const String& string, Palette* color,
				   ieByte Alignment) const
{
	Palette* pal = color;
	if (!pal) {
		pal = palette;
	}

	ieWord ystep = (Alignment&IE_FONT_SINGLE_LINE) ? StringSize(string).h : maxHeight;
	int x = 0, y = ystep;
	Video* video = core->GetVideoDriver();

	if (Alignment & (IE_FONT_ALIGN_MIDDLE|IE_FONT_ALIGN_BOTTOM)) {
		int lc = 1;
		for (size_t i = 0; i <= string.length(); i++) {
			if (string[i] == L'\n')
				lc++;
		}
		int h = lc * ystep;
		if (Alignment & IE_FONT_ALIGN_MIDDLE) {
			y += ( rgn.h - h ) / 2;
		} else {
			y += ( rgn.h - h );
		}
	}

	// is this horribly inefficient?
	std::wistringstream stream(string);
	String line, word;
	const Sprite2D* currGlyph = NULL;
	bool done = false, lineBreak = false;
	size_t charCount = 0;

	while (!done && (lineBreak || getline(stream, line))) {
		lineBreak = false;

		ieWord lineW = StringSize(line).w;
		if (Alignment & IE_FONT_ALIGN_CENTER) {
			x = ( rgn.w - lineW ) / 2;
		} else if (Alignment & IE_FONT_ALIGN_RIGHT) {
			x = ( rgn.w - lineW );
		} else {
			x = 0;
		}

		size_t lineLen = line.length();
		if (lineLen) {
			size_t linePos = 0, wordBreak = 0;
			// FIXME: I'm not sure how to handle Asian text
			// should a "word" be a single Asian glyph? that way we wouldnt clip off text (we were doing this before the rewrite too).
			// we could check the core encoding for the 'zerospace' attribute and treat single characters as words
			// that would looks funny with partial translations, however. we would need to handle both simultaniously.
			while (!lineBreak && (wordBreak = line.find_first_of(L' ', linePos))) {
				if (wordBreak == String::npos) {
					word = line.substr(linePos);
				} else {
					word = line.substr(linePos, wordBreak - linePos);
				}

				int wordW = StringSize(word).w;
				if (!(Alignment&IE_FONT_SINGLE_LINE)) {
					if (x + wordW > rgn.w && wordW != (int)lineW) {
						//assert(word != line);
						// wrap to new line, only if the word isnt >= the entire line
						lineBreak = true;
						line = line.substr(linePos);
					}
				}

				if (!lineBreak) {
					// print the word
					wchar_t currChar = '\0';
					size_t i;
					for (i = 0; i < word.length(); i++) {
						// process glyphs in word
						currChar = word[i];
						if (currChar == '\r') {
							continue;
						}
						if (i > 0) { // kerning
							x -= GetKerningOffset(word[i-1], currChar);
						}
						currGlyph = GetCharSprite(currChar);
						// should probably use rect intersection, but new lines shouldnt be to the left ever.
						if (!rgn.PointInside(x + rgn.x - currGlyph->XPos,
												 y + rgn.y - currGlyph->YPos)) {
							if (!wordW > (int)lineW) {
								// this probably doest cover every situation 100%
								// we consider printing done if the blitter is outside the region
								// *and* the word isnt wider then the line
								done = true;
							} else {
#if DEBUG_FONT
								Log(WARNING, "Font", "The word '%ls' (width=%d) overruns %d",
									word.c_str(), wordW, rgn.w);
#endif
							}
							break; // either done, or skipping
						}
						video->BlitSprite(currGlyph, x + rgn.x, y + rgn.y, true, &rgn, pal);

						x += currGlyph->Width;
					}
					x += GetCharSprite(' ')->Width;
					linePos += i + 1;
					charCount += i + 1;
				}
				if (wordBreak == String::npos) break;
			}
		}

		if (!lineBreak && !stream.eof())
			charCount++; // for the newline
		y += ystep;
	}
	// FIXME: charCount appears to sometimes be off by 1...
	//assert(charCount <= string.length());
	return charCount;
}

Size Font::StringSize(const String& string, const Size* padding) const
{
	ieWord w = 0, h = 0, lines = 1;
	ieWord curh = 0, curw = (padding) ? padding->w*2 : 0;
	bool multiline = false;
	for (size_t i = 0; i < string.length(); i++) {
		if (string[i] == L'\n') {
			if (curw > w)
				w = curw;
			curw = (padding) ? padding->w*2 : 0;
			multiline = true;
			lines++;
		} else {
			curh = GetCharSprite(string[i])->Height;
			curh += (padding) ? padding->h : 0;
			if (curh > h)
				h = curh;
			curw += GetCharSprite(string[i])->Width;
			if (i > 0) { // kerning
				curw -= GetKerningOffset(string[i-1], string[i]);
			}
		}
	}
	if (!multiline) {
		w = curw;
	} else {
		h = maxHeight * lines;
	}
	return Size(w, h);
}

void Font::SetName(const char* newName)
{
	strnlwrcpy( name, newName, sizeof(name)-1);
}

Palette* Font::GetPalette() const
{
	assert(palette);
	palette->acquire();
	return palette;
}

void Font::SetPalette(Palette* pal)
{
	if (pal) pal->acquire();
	if (palette) palette->release();
	palette = pal;
}

}
