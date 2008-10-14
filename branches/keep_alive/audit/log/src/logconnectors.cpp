#include "audit/log/logconnectors.hpp"

using namespace std;

namespace audit{

LogConnector::LogConnector():usecnt(0), mustdie(false){
}
/*virtual*/ LogConnector::~LogConnector(){
}
bool LogConnector::destroy(){
	mustdie = true;
	return usecnt == 0;
}
bool LogConnector::receivePrepare(
	LogRecorderVector& _outrv,
	const LogRecord &_rrec,
	const LogClientData &_rcl
){
	if(mustdie) return false;
	uint32 sz = _outwv.size();
	doReceive(_outwv, _rrec, _rcon);
	if(sz != _outwv.size()){
		++usecnt;
		return true;
	}
	return false;
}
bool LogConnector::receiveDone(){
	--usecnt;
	if(mustdie && !usecnt) return true;
	return false;
}

//-------------------------------------------------------------------------
struct LogBasicConnector::Data{
	struct RecorderVector: std::vector<LogRecorder*>{
		~RecorderVector();
	};
	std::string		prfx;
	RecorderVector	rv;
};

LogBasicConnector::RecorderVector::~RecorderVector(){
	for(const_iterator it(begin()); it != end(); ++it){
		delete *it;
	}
}
//-------------------------------------------------------------------------
LogBasicConnector::LogBasicConnector(const char *_prfx):d(*(new Data)) {
	prefix(_prfx);
}
void prefix(const char *_prfx){
	if(_prfx){
		d.prfx = _prfx;
		if(d.prfx.size() && d.prfx[d.prfx.size() - 1] != '/'){
			d.prfx += '/';
		}
	}else d.prfx.clear();
}
LogBasicConnector::~LogBasicConnector(){
	delete &d;
}
/*virtual*/ void LogBasicConnector::eraseClient(uint32 _idx, uint32 _uid){
	if(_idx < d.rv.size()){
		assert(d.rv[_idx]);
		delete d.rv[_idx];
		d.rv[_idx] = NULL;
	}
}
/*virtual*/ void LogBasicConnector::doReceive(
	LogRecorderVector& _outrv,
	const LogRecord &_rrec,
	const LogClientData &_rcl
){	
	if(_rcon.idx >= d.rv.size()){
		//TODO ensure there a zeros filling 
		d.rv.resize(_rcon.idx + 1);
	}
	LogRecorder *plr = d.rv[_rcon.idx];
	if(!plr){	
		plr = d.rv[_rcon.idx] = createRecorder(_rcon);
	}
	_outrv.push_back(plr);
}
LogRecorder* LogBasicConnector::createRecorder(const LogClientData &_rcl){
	string pth(d.prfx);
	pth += _rcl.procname;
	char buf[128];
	tm loctm;
	time_t t = time(NULL);
	localtime_r(&t, &loctm);
	sprintf(
		buf,
		"_%04u-%02u-%02u__%02u_%02u_%02u__%06u",
		loctm.tm_year + 1900,
		loctm.tm_mon, 
		loctm.tm_mday,
		loctm.tm_hour,
		loctm.tm_min,
		loctm.tm_sec,
		_rcl.head.procid
	);
	pth += buf;
	return new LogFileRecorder(pth.c_str());
}

}//namespace audit
