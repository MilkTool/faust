/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2019 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/

#ifndef __SIGNALSPLITTER__
#define __SIGNALSPLITTER__

#include <iostream>
#include <map>
#include <set>

#include "signals.hh"

/**
 * @brief Split a list of signals into a set of instructions
 *
 * @param conditionProperty
 * @param LS the list of signals to split
 * @return set<Tree> the set of instructions
 */

set<Tree> splitSignalsToInstr(const map<Tree, Tree>& conditionProperty, Tree LS);

#endif