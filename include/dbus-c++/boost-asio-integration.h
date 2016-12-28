/*
 *
 *  D-Bus++ - C++ bindings for D-Bus
 *
 *  Copyright (C) 2005-2007  Paolo Durante <shackan@gmail.com>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef __DBUSXX_BOOST_ASIO_INTEGRATION_H
#define __DBUSXX_BOOST_ASIO_INTEGRATION_H

#ifdef HAVE_WIN32
#error "Boost dispatcher currently has no support for Windows sockets! Please implement"
#endif

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "api.h"
#include "dispatcher.h"

namespace DBus {

namespace Boost {

class BusDispatcher;

class DXXAPI BusTimeout : public Timeout
{
private:

	BusTimeout(Timeout::Internal *, boost::asio::io_service *);

	~BusTimeout();

	void toggle();

	void enable();

	void disable();

	void callback(const boost::system::error_code &);

private:

	boost::asio::io_service *_ctx;
	boost::asio::deadline_timer *_timer;

friend class BusDispatcher;
};

class DXXAPI BusWatch : public Watch
{
private:

	BusWatch(Watch::Internal *, boost::asio::io_service *, BusDispatcher *);

	~BusWatch();

	void toggle();

	void enable();

	void disable();

	void callback(const boost::system::error_code &, size_t, int);

private:

	boost::asio::io_service *_ctx;
	BusDispatcher *_dispatcher;
	boost::asio::posix::stream_descriptor *_wch;

friend class BusDispatcher;
};

class DXXAPI BusDispatcher : public Dispatcher
{
public:

	BusDispatcher();
	~BusDispatcher();

	void attach(boost::asio::io_service *);

	void enter() {}

	void leave() {}

	Timeout *add_timeout(Timeout::Internal *);

	void rem_timeout(Timeout *);

	Watch *add_watch(Watch::Internal *);

	void rem_watch(Watch *);

private:

	boost::asio::io_service *_ctx;
};

} /* namespace Boost */

} /* namespace DBus */

#endif//__DBUSXX_BOOST_ASIO_INTEGRATION_H
