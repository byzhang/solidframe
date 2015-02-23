// frame/aio2/src/aioobject.cpp
//
// Copyright (c) 2014 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#include "system/cassert.hpp"
#include "system/mutex.hpp"
#include "system/thread.hpp"
#include "system/debug.hpp"

#include "frame/service.hpp"
#include "frame/aio2/aioobject.hpp"
#include "frame/aio2/aioreactor.hpp"
#include "frame/aio2/aiocompletion.hpp"
#include "frame/aio2/aioreactorcontext.hpp"

#include "utility/memory.hpp"
#include "utility/dynamicpointer.hpp"

namespace solid{
namespace frame{
namespace aio{
//---------------------------------------------------------------------
//----	Object	----
//---------------------------------------------------------------------

Object::Object(){}

/*virtual*/ void Object::onEvent(ReactorContext &_rctx, Event const &_revent){
	
}

void Object::postStop(ReactorContext &_rctx){
	
}

bool Object::isRunning()const{
	return runId().isValid();
}

void Object::registerCompletionHandler(CompletionHandler &_rch){
	_rch.pnext = this->pnext;
	if(_rch.pnext){
		_rch.pnext->pprev = &_rch;
	}
	this->pnext = &_rch;
	_rch.pprev = this;
}

void Object::registerCompletionHandlers(ReactorContext &_rctx){
	CompletionHandler *pch = this->pnext;
	
	while(pch != nullptr){
		pch->activate(_rctx);
		pch = pch->pnext;
	}
}

void Object::doPost(ReactorContext &_rctx, EventFunctionT &_revfn, Event const &_revent){
	_rctx.reactor().post(_rctx, _revfn, _revent);
}

}//namespace aio
}//namespace frame
}//namespace solid
