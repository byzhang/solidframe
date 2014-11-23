// frame/aio/reactor.hpp
//
// Copyright (c) 2014 Valentin Palade (vipalade @ gmail . com) 
//
// This file is part of SolidFrame framework.
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.
//
#ifndef SOLID_FRAME_AIO_REACTOR_HPP
#define SOLID_FRAME_AIO_REACTOR_HPP

#include "system/timespec.hpp"
#include "frame/common.hpp"
#include "frame/reactorbase.hpp"
#include "utility/dynamicpointer.hpp"
#include "frame/aio2/aiocommon.hpp"

namespace solid{

struct TimeSpec;

namespace frame{
namespace aio{

class Object;
struct ReactorContext;
struct CompletionHandler;

typedef DynamicPointer<Object>	ObjectPointerT;

//! 
/*!
	
*/
class Reactor: public frame::ReactorBase{
public:
	typedef ObjectPointerT		JobT;
	typedef Object				ObjectT;
	
	Reactor(SchedulerBase &_rsched);
	~Reactor();
	
	static Reactor* safeSpecific();
	static Reactor& specific();
	
	template <typename F>
	void post(ReactorContext &_rctx, F _f, Event const &_rev, CompletionHandler *_pch){
		
	}
	
	void wait(ReactorContext &_rctx, CompletionHandler *_pch, ReactorWaitRequestsE _req);
	
	/*virtual*/ bool raise(UidT const& _robjuid, Event const& _re);
	/*virtual*/ void stop();
	/*virtual*/ void update();
	
	void registerCompletionHandler(CompletionHandler &_rch);
	void unregisterCompletionHandler(CompletionHandler &_rch);
	
	void run();
	bool push(JobT &_rcon);
private:
private://data
	struct Data;
	Data	&d;
};


}//namespace aio
}//namespace frame
}//namespace solid

#endif
