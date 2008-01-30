/* Declarations file streamptr.h
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidGround framework.

	SolidGround is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Foobar is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidGround.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STREAM_PTR_H
#define STREAM_PTR_H

class Stream;

struct StreamPtrBase{
protected:
	void clear(Stream *_pobj);
};


template <class SO>
class StreamPtr: StreamPtrBase{
public:
	typedef SO 				ObjectTp;
	typedef StreamPtr<SO>	ThisTp;
	StreamPtr():pobj(NULL){}
	StreamPtr(ObjectTp *_pobj):pobj(_pobj) {}
	StreamPtr(const ThisTp &_pobj):pobj(_pobj.release()){}
	~StreamPtr(){
		if(pobj){StreamPtrBase::clear(pobj);}
	}
	ObjectTp* release() const{
		ObjectTp *po = pobj;
		pobj = NULL;//TODO:
		return po;
	}
	ThisTp& operator=(const ThisTp &_robj){
		if(pobj) clear();
		pobj = _robj.release();
		return *this;
	}
	ThisTp& operator=(ObjectTp *_pobj){
		pobj = _pobj;
		return *this;
	}
	inline ObjectTp& operator*()const throw() {return *pobj;}
	inline ObjectTp* operator->()const throw() {return pobj;}
	ObjectTp* ptr() const throw() {return pobj;}
	operator bool () const throw() {return pobj;}
	bool operator!()const throw() {return !pobj;}
	void clear(){if(pobj){StreamPtrBase::clear(pobj);pobj = NULL;}}
private:
	mutable ObjectTp 	*pobj;
};

#endif
