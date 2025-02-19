// Copyright 2023 Northern.tech AS
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#ifndef MENDER_COMMON_IO_UTIL_HPP
#define MENDER_COMMON_IO_UTIL_HPP

#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include <common/events.hpp>
#include <common/io.hpp>

namespace mender {
namespace common {
namespace events {
namespace io {

using namespace std;

namespace asio = boost::asio;
namespace mio = mender::common::io;

enum class Append {
	Disabled,
	Enabled,
};

class AsyncFileDescriptorReader : public EventLoopObject, virtual public mio::AsyncReader {
public:
	// Takes ownership of fd.
	explicit AsyncFileDescriptorReader(events::EventLoop &loop, int fd);
	explicit AsyncFileDescriptorReader(events::EventLoop &loop);
	~AsyncFileDescriptorReader();

	error::Error Open(const string &path);

	error::Error AsyncRead(
		vector<uint8_t>::iterator start,
		vector<uint8_t>::iterator end,
		mio::AsyncIoHandler handler) override;
	void Cancel() override;

private:
#ifdef MENDER_USE_BOOST_ASIO
	asio::posix::stream_descriptor pipe_;
	shared_ptr<bool> destroying_;
#endif // MENDER_USE_BOOST_ASIO
};
using AsyncFileDescriptorReaderPtr = shared_ptr<AsyncFileDescriptorReader>;

class AsyncFileDescriptorWriter : public EventLoopObject, virtual public mio::AsyncWriter {
public:
	// Takes ownership of fd.
	explicit AsyncFileDescriptorWriter(events::EventLoop &loop, int fd);
	explicit AsyncFileDescriptorWriter(events::EventLoop &loop);
	~AsyncFileDescriptorWriter();

	error::Error Open(const string &path, Append append = Append::Disabled);

	error::Error AsyncWrite(
		vector<uint8_t>::const_iterator start,
		vector<uint8_t>::const_iterator end,
		mio::AsyncIoHandler handler) override;
	void Cancel() override;

private:
#ifdef MENDER_USE_BOOST_ASIO
	asio::posix::stream_descriptor pipe_;
	shared_ptr<bool> destroying_;
#endif // MENDER_USE_BOOST_ASIO
};
using AsyncFileDescriptorWriterPtr = shared_ptr<AsyncFileDescriptorWriter>;

class AsyncReaderFromReader : virtual public mio::AsyncReader {
public:
	AsyncReaderFromReader(EventLoop &loop, mio::ReaderPtr reader);
	~AsyncReaderFromReader();

	error::Error AsyncRead(
		vector<uint8_t>::iterator start,
		vector<uint8_t>::iterator end,
		mio::AsyncIoHandler handler) override;
	// Important: There is no way to cancel a Read operation on a normal Reader, so `Cancel()`
	// will assert if a Read is in progress.
	void Cancel() override;

private:
	bool in_progress_ {false};
	shared_ptr<bool> cancelled_;
	mio::ReaderPtr reader_;
	EventLoop &loop_;
};

class AsyncWriterFromWriter : virtual public mio::AsyncWriter {
public:
	AsyncWriterFromWriter(EventLoop &loop, mio::WriterPtr writer);
	~AsyncWriterFromWriter();

	error::Error AsyncWrite(
		vector<uint8_t>::const_iterator start,
		vector<uint8_t>::const_iterator end,
		mio::AsyncIoHandler handler) override;
	// Important: There is no way to cancel a Write operation on a normal Writer, so `Cancel()`
	// will assert if a Write is in progress.
	void Cancel() override;

private:
	bool in_progress_ {false};
	shared_ptr<bool> cancelled_;
	mio::WriterPtr writer_;
	EventLoop &loop_;
};

using AsyncReaderFromEventLoopFunc = function<mio::ExpectedAsyncReaderPtr(EventLoop &loop)>;

class ReaderFromAsyncReader : virtual public mio::Reader {
public:
	// Note that it is not possible to use Cancel on the AsyncReader, or destroy it, before Read
	// has returned, so be careful with this!
	ReaderFromAsyncReader(EventLoop &event_loop, mio::AsyncReaderPtr reader);
	ReaderFromAsyncReader(EventLoop &event_loop, mio::AsyncReader &reader);

	mio::ExpectedSize Read(vector<uint8_t>::iterator start, vector<uint8_t>::iterator end) override;

private:
	EventLoop &event_loop_;

	mio::AsyncReaderPtr reader_;
};

} // namespace io
} // namespace events
} // namespace common
} // namespace mender

#endif // MENDER_COMMON_IO_UTIL_HPP
