/* Implementation file stream.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidGround framework.

	SolidGround is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SolidGround is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidGround.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include "stream.hpp"
#include "istream.hpp"
#include "ostream.hpp"
#include "iostream.hpp"
#include "streamptr.hpp"

void StreamPtrBase::clear(Stream *_ps){
	if(_ps->release()) delete _ps;
}

Stream::~Stream(){
}

int Stream::release(){return -1;}
//bool Stream::isOk()const{return true;}

int64 Stream::size()const{return -1;}

IStream::~IStream(){
}

OStream::~OStream(){
}

IOStream::~IOStream(){
}

#ifndef UINLINES
#include "stream.ipp"
#include "istream.ipp"
#include "ostream.ipp"
#include "iostream.ipp"
#endif

