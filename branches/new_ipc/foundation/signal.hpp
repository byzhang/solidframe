/* Declarations file signal.hpp
	
	Copyright 2007, 2008, 2009 Valentin Palade 
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

#ifndef CS_SIGNAL_HPP
#define CS_SIGNAL_HPP

#include "foundation/common.hpp"

#include "utility/streampointer.hpp"
#include "utility/dynamictype.hpp"

#include <string>

class TimeSpec;

template <class T>
class DynamicPointer;


namespace foundation{

namespace ipc{
	struct ConnectionUid;
	struct SignalUid;
}

class SignalExecuter;
class Object;
//! A base class for signals to be sent to objects
/*!
	Inherit this class if you want so send something to an object.
	Implement the proper execute method.
	Also the signal can be used with an foundation::SignalExecuter,
	in which case the posibilities widen.
	\see test::alpha::FetchMasterSignal
*/
struct Signal: Dynamic<Signal>{
	Signal();
	virtual ~Signal();
	//! Called by ipc module after the signal was successfully parsed
	/*!
		\retval BAD for deleting the signal, OK for not
	*/
	virtual int ipcReceived(ipc::SignalUid &, const ipc::ConnectionUid&);
	//! Called by ipc module, before the signal begins to be serialized
	virtual int ipcPrepare(const ipc::SignalUid&);
	//! Called by ipc module on peer failure detection (disconnect,reconnect)
	virtual void ipcFail(int _err);
	
	//! Called by the SignalExecuter
	virtual int execute(uint32 _evs, SignalExecuter &_rce, const SignalUidT &_rcmduid, TimeSpec &_rts);

	//! Called by the SignalExecuter when receiving a signal for a signal
	/*!
		If it returns OK, the signal is rescheduled for execution
	*/
	virtual int receiveSignal(
		DynamicPointer<Signal> &_rsig,
		const ObjectUidT& _from = ObjectUidT(),
		const ipc::ConnectionUid *_conid = NULL
	);
};
}//namespace foundation

#endif