#ifndef SYSTEM_DYNAMIC_HPP
#define SYSTEM_DYNAMIC_HPP

#include "system/common.hpp"

struct DynamicId{
	static uint32 generate();
	DynamicId(uint32 _v):v(_v){}
	uint32	v;
};

struct DynamicMap{
	typedef int (*FncTp)(void*,void*);
	typedef void (*RegisterFncTp)(DynamicMap &_rdm);
	static uint32 generateId();
	
	DynamicMap(RegisterFncTp _prfnc);
	~DynamicMap();
	void callback(uint32 _tid, FncTp _pf);
	FncTp callback(uint32 _id);
	struct Data;
	Data	&d;
};

struct DynamicBase{
	virtual uint32 dynamicTypeId()const = 0;
protected:
	virtual ~DynamicBase();
};

template <class X, class T = DynamicBase>
struct Dynamic: T{
	typedef Dynamic<X,T>	BaseTp;
	Dynamic(){}
	template<class G>
	Dynamic(G _x):T(_x){}
	static uint32 staticTypeId(){
		static uint32 id(DynamicMap::generateId());
		return id;
	}
	virtual uint32 dynamicTypeId()const{
		return staticTypeId();
	}
};

struct DynamicReceiverBase{
	typedef int (*FncTp)(void*,void*);
};


template <class X, class T = DynamicReceiverBase>
struct DynamicReceiver: T{
	typedef DynamicReceiver<X,T>	BaseTp;
	DynamicReceiver(){}
	template<class G>
	DynamicReceiver(G _x):T(_x){}

	template <class C>
	static int dynamicCallback(void *_pdyn, void *_prcv){
		return static_cast<X*>(_prcv)->dynamicReceive(*static_cast<C*>(_pdyn));
	}
	static DynamicMap& dynamicMap(){
		static DynamicMap dm(&X::dynamicRegister);
		return dm;
	}
	template <class C>
	static void dynamicRegister(DynamicMap &_rdm){
		_rdm.callback(C::staticTypeId(), &dynamicCallback<C>);
	}
	int dynamicReceiver(DynamicBase *_pd){
		DynamicMap::FncTp pf = dynamicMap().callback(_pd->dynamicTypeId());
		if(pf) return (*pf)(_pd, this);
		return BAD;
	}
};

#endif
