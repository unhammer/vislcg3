/*
* Copyright (C) 2007, GrammarSoft Aps
* Developed by Tino Didriksen <tino@didriksen.cc>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <tino@didriksen.cc>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __COMPOSITETAG_H
#define __COMPOSITETAG_H

#include "stdafx.h"
#include "Tag.h"

namespace CG3 {

	class CompositeTag {
	public:
		uint32_t hash;
		uint32Set tags_set;
		uint32HashSet tags;

		CompositeTag();
		~CompositeTag();

		void addTag(uint32_t tag);

		uint32_t rehash();
	};

}

#endif
