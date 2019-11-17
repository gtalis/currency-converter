/*
 *  Copyright (c) 2019 Gilles Talis
 *
 *	This file is part of CurrencyConverter.
 *
 *	CurrencyConverter is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	CurrencyConverter is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CC_UTILS_H
#define CC_UTILS_H

#include "rateManager.h"
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <fstream>

std::string _getStorageFileName();
std::string _getLastUpdatedFileName();
time_t ecbDateToTime(const char *date);
time_t getEcbLastUpdateTime();

#endif
