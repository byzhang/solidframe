#include "dchatmessagematrix.hpp"
#include "system/mutex.hpp"
#include <sstream>
#include <map>
#include <memory>
#include <deque>
#include "system/debug.hpp"

using namespace std;
using namespace solid;

TextMessage::TextMessage(const std::string &_txt):TextMessageBase(_txt){
	idbg((void*)this);
}
TextMessage::TextMessage(){
	idbg((void*)this);
}
TextMessage::~TextMessage(){
	idbg((void*)this);
}

namespace{
	string create_char_set(){
		string s;
		for(char c = '0'; c <= '9'; ++c){
			s += c;
		}
		for(char c = 'a'; c <= 'z'; ++c){
			s += c;
		}
		for(char c = 'A'; c <= 'Z'; ++c){
			s += c;
		}
		return s;
	}
	void create_text(ostream &_ros, size_t _from, size_t _to, size_t _count, size_t _idx){
		static const string char_set = create_char_set();
		
		if(!_count) return;
		
		if(_from > _to){
			size_t tmp = _to;
			_to = _from;
			_from = tmp;
			_idx = _count - 1 - _idx;
		}
		const size_t newsz = (_count > 1) ? ((_from * (_count - _idx - 1) + _idx * _to) / (_count - 1)) : _from;
		//const size_t oldsz = _rs.size();
		for(size_t i = 0; i < newsz; ++i){
			_ros << char_set[i % char_set.size()];
		}
	}
	
}//namespace

typedef std::deque<TextMessagePointerT>							TextMessageVectorT;
typedef std::map<size_t, std::unique_ptr<TextMessageVectorT> >	TextMessageMapT;

struct MessageMatrix::Data{
	TextMessageMapT				tmmap;
	mutable SharedMutex			shrmtx;
};

MessageMatrix::MessageMatrix():d(*(new Data)){}
MessageMatrix::~MessageMatrix(){
	delete &d;
}

void MessageMatrix::createRow(
	const size_t _row_idx,
	const size_t _cnt,
	const size_t _from_sz,
	const size_t _to_sz
){
	Locker<SharedMutex>	lock(d.shrmtx);
	d.tmmap[_row_idx] = std::unique_ptr<TextMessageVectorT>(new TextMessageVectorT);
	TextMessageVectorT	&rvec = *d.tmmap[_row_idx];
	
	for(size_t i = 0; i < _cnt; ++i){
		ostringstream oss;
		oss<<_row_idx<<' '<<i<<' ';
		create_text(oss, _from_sz, _to_sz, _cnt, i);
		rvec.push_back(new TextMessage(oss.str()));
	}
}

void MessageMatrix::printRowInfo(std::ostream &_ros, const size_t _row_idx, const bool _verbose)const {
	_ros<<"Messages on row "<<_row_idx<<endl;
	SharedLocker<SharedMutex>	lock(d.shrmtx);
	const TextMessageVectorT	&rvec = *d.tmmap[_row_idx];
	for(TextMessageVectorT::const_iterator it(rvec.begin()); it != rvec.end(); ++it){
		const TextMessage &rtm = *(*it);
		if(_verbose){
			_ros<<'['<<rtm.text.size()<<':'<<rtm.text<<']'<<endl;
		}else{
			_ros<<'['<<rtm.text.size()<<']';
		}
	}
	_ros<<endl;
}

TextMessagePointerT MessageMatrix::message(const size_t _row_idx, const size_t _idx)const{
	return TextMessagePointerT();
}