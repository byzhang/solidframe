#include "audit/log/logrecorders.hpp"
#include "audit/log/logclientdata.hpp"
#include "audit/log/logrecord.hpp"
#include <ctime>
namespace audit{

//--------------------------------------------------------
LogRecorder::~LogRecorder(){
}
//--------------------------------------------------------
int LogFileRecorder::open(const char *_name){
	if(_name){
		name = _name;
	}
	string path = name;
	path += ".txt";
	ofs.open(path.c_str());
	return OK;
}
LogFileRecorder::~LogFileRecorder(){
}

static const char* levelName(unsigned _lvl){
	switch(_lvl){
		case Log::Warn:		return "WARN";
		case Log::Info:		return "INFO";
		case Log::Error:	return "ERROR";
		case Log::Debug:	return "DEBUG";
		case Log::Input:	return "INPUT";
		case Log::Output:	return "OUTPUT";
		default: return "UNKNOWN";
	}
}

/*virtual*/ void LogFileRecorder::record(const LogClientData &_rcd, const LogRecord &_rrec){
	char buf[128];
	tm loctm;
	time_t sec = _rrec.head.sec;
	localtime_r(&sec, &loctm);
	sprintf(
		buf,
		"%s[%04u-%02u-%02u %02u:%02u:%02u.%03u]",
		levelName(_rrec.head.level),
		loctm.tm_year + 1900,
		loctm.tm_mon, 
		loctm.tm_mday,
		loctm.tm_hour,
		loctm.tm_min,
		loctm.tm_sec,
		_rrec.head.nsec/1000000
	);

	ofs<<buf<<'['<<_rcd.modulenamev[_rrec.head.module]<<']';
	ofs<<'['<<_rrec.fileName()<<'('<<_rrec.head.lineno<<')'<<' '<<_rrec.functionName()<<']'<<'['<<_rrec.head.id<<']'<<' ';
	ofs.write(_rrec.data(), _rrec.dataSize());
}


}//namespace audit
