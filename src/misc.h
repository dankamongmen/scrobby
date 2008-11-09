/***************************************************************************
 *   Copyright (C) 2008 by Andrzej Rybczak   *
 *   electricityispower@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _MISC_H
#define _MISC_H

#include <sstream>
#include <string>
#include <vector>

#include "configuration.h"

size_t write_data(char *, size_t, size_t, std::string);

void ChangeToUser();

bool Daemonize();

void ClearCache();
void GetCachedSongs(std::vector<std::string> &);

void Cache(const std::string &);
void Log(LogLevel ll, const char *, ...);

void ignore_newlines(std::string &);

std::string md5sum(const std::string &);

std::string DateTime();

int StrToInt(const std::string &);

template <class T>
std::string IntoStr(T t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

#endif

