/* Declarations file iostream.hpp
	
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

#ifndef IOSTREAMPP_HPP
#define IOSTREAMPP_HPP

#include "istream.hpp"
#include "ostream.hpp"

//! A stream for both input and output
class IOStream: public IStream, public OStream{
public:
	virtual ~IOStream();
};
//! An IOStreamIterator - an offset within the stream: a pointer to an iostream
struct IOStreamIterator{
	IOStreamIterator(IOStream *_ps = NULL, int64 _off = 0);
	void reinit(IOStream *_ps = NULL, int64 _off = 0);
	int64 start();
	IOStream* operator->() const{return ps;}
	IOStream& operator*() {return *ps;}
	IOStream	*ps;
	int64		off;
};

#ifndef NINLINES
#include "src/iostream.ipp"
#endif

#endif
