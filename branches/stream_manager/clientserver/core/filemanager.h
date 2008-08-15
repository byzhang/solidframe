/* Declarations file filemanager.h
	
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

#ifndef CS_FILE_MANAGER_H
#define CS_FILE_MANAGER_H

#include "utility/streamptr.h"

#include "clientserver/core/object.h"

#include "common.h"

class IStream;
class OStream;
class IOStream;

class Mutex;

namespace clientserver{
class File;


struct FileKey;
struct FileMapper;
class FileIStream;
class FileOStream;
class FileIOStream;
class File;
/*!
	NOTE:
	1) The mapper method is a little bit more complex than it should be (doGetMapper must also register a mapper) because I want to be able to have more than one instance of FileManager per process. It is not a really usefull request, but as long as one can have multiple servers per process it is at least nice to be able to have multiple file managers.
	2) Some words about Forced flag:
		# Forced flag can only be used to request a stream for an existing file, opened (see below the fourth type of stream request methods)
		# If the file is not opened BAD is the return value.
		# It will return an io/ostream even if there are readers, or an istream even if there is a writer.
		# It will return BAD if there already is a writer!
	The force flag is designed for situations like:
		# you request an istream for a (temp) file, give the stream id to someone else which will forcedly request an ostream for that id, write data on it, close the ostream and then signal you that you can start reading.
		# think of a log file someone has a writer and you want a reader to fetch log records.
*/

class FileManager: public Object{
public:
	struct RequestUid{
		RequestUid(uint32 _objidx = 0, uint32 _objuid = 0, uint32 _reqidx = 0, uint32 _requid = 0):objidx(_objidx), objuid(_objuid), reqidx(_reqidx), requid(_requid){}
		uint32	objidx;
		uint32	objuid;
		uint32	reqidx;
		uint32	requid;
	};
	enum {
		Forced = 1,
		Create = 2,
		IOStreamRequest = 4,
		NoWait = 8,
		ForcePending = 16,
	};
	//typedef std::pair<uint32, uint32> FileUidTp;
	
	FileManager(uint32 _maxfcnt = 0);
	~FileManager();
	
	int execute(ulong _evs, TimeSpec &_rtout);
	
	int64 fileSize(const char *_fn)const;
	int64 fileSize(const FileKey &_rk)const;
	template <class T>
	T* mapper(T *_pm = NULL){
		//doRegisterMapper will assert if _pm is NULL
		static const int id(doRegisterMapper(static_cast<FileMapper*>(_pm)));
		//doGetMapper may also register the mapper - this way one can instantiate more managers per process.
		return static_cast<T*>(doGetMapper(id, static_cast<FileMapper*>(_pm)));
	}
	//first type of stream request methods
	int stream(StreamPtr<IStream> &_sptr, const RequestUid &_rrequid, const char *_fn = NULL, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, const RequestUid &_rrequid, const char *_fn = NULL, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, const RequestUid &_rrequid, const char *_fn = NULL, uint32 _flags = 0);
	
	//second type of stream request methods
	int stream(StreamPtr<IStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const char *_fn, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const char *_fn, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const char *_fn, uint32 _flags = 0);
	
	//third type of stream request methods
	int stream(StreamPtr<IStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const FileKey &_rk, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const FileKey &_rk, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, FileUidTp &_rfuid, const RequestUid &_rrequid, const FileKey &_rk, uint32 _flags = 0);
	
	//fourth type of stream request methods
	int stream(StreamPtr<IStream> &_sptr, const FileUidTp &_rfuid, const RequestUid &_rrequid, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, const FileUidTp &_rfuid, const RequestUid &_rrequid, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, const FileUidTp &_rfuid, const RequestUid &_rrequid, uint32 _flags = 0);
	
	//fifth type of stream request methods
	int stream(StreamPtr<IStream> &_sptr, const char *_fn = NULL, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, const char *_fn = NULL, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, const char *_fn = NULL, uint32 _flags = 0);
	
	int stream(StreamPtr<IStream> &_sptr, const FileKey &_rk, uint32 _flags = 0);
	int stream(StreamPtr<OStream> &_sptr, const FileKey &_rk, uint32 _flags = 0);
	int stream(StreamPtr<IOStream> &_sptr, const FileKey &_rk, uint32 _flags = 0);

	int setFileTimeout(const FileUidTp &_rfuid, const TimeSpec &_rtout);
	
	//overload from object
	void mutex(Mutex *_pmut);
	uint32 fileOpenTimeout()const;
	
protected:
	virtual void sendStream(StreamPtr<IStream> &_sptr, const FileUidTp &_rfuid, const RequestUid& _rrequid) = 0;
	virtual void sendStream(StreamPtr<OStream> &_sptr, const FileUidTp &_rfuid, const RequestUid& _rrequid) = 0;
	virtual void sendStream(StreamPtr<IOStream> &_sptr, const FileUidTp &_rfuid, const RequestUid& _rrequid) = 0;
	virtual void sendError(int _error, const RequestUid& _rrequid) = 0;
private:
	friend class FileIStream;
	friend class FileOStream;
	friend class FileIOStream;
	friend class File;
	void releaseIStream(uint _fileid);
	void releaseOStream(uint _fileid);
	void releaseIOStream(uint _fileid);
	
	int doRegisterMapper(FileMapper *_pm);
	FileMapper* doGetMapper(unsigned _id, FileMapper *_pm);
	int execute();
	struct Data;
	Data	&d;
};

}//namespace clientserver

#endif