/*
 * Copyright (C) 2006, GrammarSoft Aps
 * and the VISL project at the University of Southern Denmark.
 * All Rights Reserved.
 *
 * The contents of this file are subject to the GrammarSoft Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.grammarsoft.com/GSPL or
 * http://visl.sdu.dk/GSPL.txt
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 */
#include "stdafx.h"
#include <unicode/ustring.h>
#include "Grammar.h"
#include "CompositeTag.h"
#include "Strings.h"

using namespace CG3;

Set::Set() {
	name = 0;
	line = 0;
}

Set::~Set() {
	if (name) {
		delete name;
	}
}

void Set::setName(uint32_t to) {
	if (!to) {
		to = (uint32_t)rand();
	}
	name = new UChar[32];
	memset(name, 0, 32);
	u_sprintf(name, "_G_%u_%u_", line, to);
}
void Set::setName(const UChar *to) {
	if (to) {
		name = new UChar[u_strlen(to)+1];
		u_strcpy(name, to);
	} else {
		setName((uint32_t)rand());
	}
}
const UChar *Set::getName() {
	return name;
}

void Set::setLine(uint32_t to) {
	line = to;
}
uint32_t Set::getLine() {
	return line;
}

void Set::addCompositeTag(CompositeTag *tag) {
	if (tag && tag->tags.size()) {
		tags[tag->rehash()] = tag;
	} else {
		u_fprintf(ux_stderr, "Error: Attempted to add empty tag to set!\n");
	}
}
