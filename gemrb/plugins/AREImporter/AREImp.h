/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/AREImporter/AREImp.h,v 1.22 2005/02/24 16:45:16 avenger_teambg Exp $
 *
 */

#ifndef AREIMP_H
#define AREIMP_H

#include "../Core/MapMgr.h"

class AREImp : public MapMgr {
private:
	DataStream* str;
	bool autoFree;
	ieResRef WEDResRef;
	ieDword LastSave;
	ieDword AreaFlags;
	ieWord  AreaType, WRain, WSnow, WFog, WLightning, WUnknown;
	ieDword ActorOffset, AnimOffset, AnimCount;
	ieDword VerticesOffset;
	ieDword DoorsCount, DoorsOffset;
	ieDword ExploredBitmapSize, ExploredBitmapOffset;
	ieDword EntrancesOffset, EntrancesCount;
	ieDword SongHeader, RestHeader;
	ieWord  ActorCount, VerticesCount, AmbiCount;
	ieWord  ContainersCount, InfoPointsCount, ItemsCount;
	ieDword VariablesCount;
	ieDword NoteCount;
	ieDword ContainersOffset, InfoPointsOffset, ItemsOffset;
	ieDword AmbiOffset, VariablesOffset;
	ieDword NoteOffset;
	ieResRef Script;
public:
	AREImp(void);
	~AREImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Map* GetMap(const char* ResRef);
	void release(void)
	{
		delete this;
	}
private:
	CREItem* GetItem();
};

#endif
