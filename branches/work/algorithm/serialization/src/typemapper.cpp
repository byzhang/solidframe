/* Implementation file binary.cpp
	
	Copyright 2007, 2008 Valentin Palade 
	vipalade@gmail.com

	This file is part of SolidFrame framework.

	SolidFrame is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	SolidFrame is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with SolidFrame.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <deque>
#include <vector>

#include "system/common.hpp"
#include "system/exception.hpp"

#ifdef HAVE_CPP11
#include <unordered_map>
#else
#include <map>
#endif

#include "system/cassert.hpp"
#include "system/mutex.hpp"
#include "algorithm/serialization/idtypemapper.hpp"


namespace serialization{
//================================================================
struct TypeMapperBase::Data{
	struct FunctionStub{
		FunctionStub():pf(NULL), name(NULL), id(-1){}
		FncT 		pf;
		const char 	*name;
		uint32		id;
	};
	struct PointerHash{
		size_t operator()(const char* const & _p)const{
			return reinterpret_cast<size_t>(_p);
		}
	};
	struct PointerCmpEqual{
		bool operator()(const char * const &_p1, const char * const &_p2)const{
			return _p1 == _p2;
		}
	};
	struct PointerCmpLess{
		bool operator()(const char * const &_p1, const char * const &_p2)const{
			return _p1 < _p2;
		}
	};
	typedef std::deque<FunctionStub>	FunctionVectorT;
#ifdef HAVE_CPP11
	typedef std::unordered_map<const char *, size_t, PointerHash, PointerCmpEqual>		FunctionStubMapT;
#else
	typedef std::map<const char*, size_t, PointerCmpLess>								FunctionStubMapT;
#endif

	Data():crtpos(0){}
	~Data();
	Mutex				mtx;
	FunctionVectorT		fncvec;
	FunctionStubMapT	fncmap;
	size_t				crtpos;
};
//================================================================
TypeMapperBase::Data::~Data(){
	
}
TypeMapperBase::TypeMapperBase():d(*(new Data())){
}

TypeMapperBase::~TypeMapperBase(){
	delete &d;
}

TypeMapperBase::FncT TypeMapperBase::function(const uint32 _id, uint32* &_rpid)const{
	Mutex::Locker				lock(d.mtx);
	const Data::FunctionStub	&rfs(d.fncvec[_id]);
	
	_rpid = const_cast<uint32*>(&rfs.id);
	
	return rfs.pf;
}
TypeMapperBase::FncT TypeMapperBase::function(const char *_pid, uint32* &_rpid)const{
	Mutex::Locker				lock(d.mtx);
	const Data::FunctionStub	&rfs(d.fncvec[d.fncmap[_pid]]);
	
	_rpid = const_cast<uint32*>(&rfs.id);
	
	return rfs.pf;
}
TypeMapperBase::FncT TypeMapperBase::function(const uint32 _id)const{
	Mutex::Locker				lock(d.mtx);
	const CRCIndex<uint32>		crcidx(_id, true);
	const uint32				idx(crcidx.index());
	if(idx < d.fncvec.size()){
		const Data::FunctionStub	&rfs(d.fncvec[idx]);
		if(_id == rfs.id){
			return rfs.pf;
		}
	}
	return NULL;
}

TypeMapperBase::FncT TypeMapperBase::function(const uint16 _id)const{
	const CRCIndex<uint16>		crcidx(_id, true);
	const uint16				idx(crcidx.index());
	if(idx < d.fncvec.size()){
		const Data::FunctionStub	&rfs(d.fncvec[idx]);
		if(_id == rfs.id){
			return rfs.pf;
		}
	}
	return NULL;
}

TypeMapperBase::FncT TypeMapperBase::function(const uint8  _id)const{
	const CRCIndex<uint8>		crcidx(_id, true);
	const uint8					idx(crcidx.index());
	if(idx < d.fncvec.size()){
		const Data::FunctionStub	&rfs(d.fncvec[idx]);
		if(_id == rfs.id){
			return rfs.pf;
		}
	}
	return NULL;
}

uint32 TypeMapperBase::insertFunction(FncT _f, uint32 _pos, const char *_name){
	Mutex::Locker lock(d.mtx);
	if(_pos == (uint32)-1){
		while(d.crtpos < d.fncvec.size() && d.fncvec[d.crtpos].pf != NULL){
			++d.crtpos;
		}
		if(d.crtpos == d.fncvec.size()){
			d.fncvec.push_back(Data::FunctionStub());
		}
		_pos = d.crtpos;
		++d.crtpos;
	}else{
		if(_pos >= d.fncvec.size()){
			d.fncvec.resize(_pos + 1);
		}
		if(d.fncvec[_pos].pf){
			THROW_EXCEPTION_EX("Overlapping identifiers", _pos);
			return _pos;
		}
	}
	
	CRCIndex<uint32>	crcval(_pos);
	if(!crcval.ok()){
		THROW_EXCEPTION_EX("Invalid CRCValue", _pos);
	}
	
	Data::FunctionStub	&rfs(d.fncvec[_pos]);
	rfs.pf = _f;
	rfs.name = _name;
	rfs.id = crcval.value();
	if(d.fncmap.find(_name) == d.fncmap.end()){
		d.fncmap[rfs.name] = _pos;
	}
	return rfs.id;
}

uint32 TypeMapperBase::insertFunction(FncT _f, uint16 _pos, const char *_name){
	Mutex::Locker lock(d.mtx);
	if(_pos == (uint32)-1){
		while(d.crtpos < d.fncvec.size() && d.fncvec[d.crtpos].pf != NULL){
			++d.crtpos;
		}
		if(d.crtpos == d.fncvec.size()){
			d.fncvec.push_back(Data::FunctionStub());
		}
		_pos = d.crtpos;
		++d.crtpos;
	}else{
		if(_pos >= d.fncvec.size()){
			d.fncvec.resize(_pos + 1);
		}
		if(d.fncvec[_pos].pf){
			THROW_EXCEPTION_EX("Overlapping identifiers", _pos);
			return _pos;
		}
	}
	
	CRCIndex<uint16>	crcval(_pos);
	
	if(!crcval.ok()){
		THROW_EXCEPTION_EX("Invalid CRCValue", _pos);
	}
	
	Data::FunctionStub	&rfs(d.fncvec[_pos]);
	rfs.pf = _f;
	rfs.name = _name;
	rfs.id = crcval.value();
	if(d.fncmap.find(_name) == d.fncmap.end()){
		d.fncmap[rfs.name] = _pos;
	}
	return rfs.id;
}

uint32 TypeMapperBase::insertFunction(FncT _f, uint8  _pos, const char *_name){
	Mutex::Locker lock(d.mtx);
	if(_pos == (uint8)-1){
		while(d.crtpos < d.fncvec.size() && d.fncvec[d.crtpos].pf != NULL){
			++d.crtpos;
		}
		if(d.crtpos == d.fncvec.size()){
			d.fncvec.push_back(Data::FunctionStub());
		}
		_pos = d.crtpos;
		++d.crtpos;
	}else{
		if(_pos >= d.fncvec.size()){
			d.fncvec.resize(_pos + 1);
		}
		if(d.fncvec[_pos].pf){
			THROW_EXCEPTION_EX("Overlapping identifiers", _pos);
			return _pos;
		}
	}
	
	CRCIndex<uint8>	crcval(_pos);
	if(!crcval.ok()){
		THROW_EXCEPTION_EX("Invalid CRCValue", _pos);
	}
	
	Data::FunctionStub	&rfs(d.fncvec[_pos]);
	rfs.pf = _f;
	rfs.name = _name;
	rfs.id = crcval.value();
	if(d.fncmap.find(_name) == d.fncmap.end()){
		d.fncmap[rfs.name] = _pos;
	}
	return rfs.id;
}

/*virtual*/ void TypeMapperBase::prepareStorePointer(
	void *_pser, void *_p,
	uint32 _rid, const char *_name
)const{
}

/*virtual*/ void TypeMapperBase::prepareStorePointer(
	void *_pser, void *_p,
	const char *_pid, const char *_name
)const{
}

/*virtual*/ bool TypeMapperBase::prepareParsePointer(
	void *_pdes, std::string &_rs,
	void *_p, const char *_name
)const{
	return false;
}
/*virtual*/ void TypeMapperBase::prepareParsePointerId(
	void *_pdes, std::string &_rs,
	const char *_name
)const{
	
}

}//namespace serialization
