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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <dbus-c++/boost-asio-integration.h>

#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <dbus/dbus.h> // for DBUS_WATCH_*

using namespace DBus;

Boost::BusTimeout::BusTimeout(Timeout::Internal *ti, boost::asio::io_service *ctx)
: Timeout(ti), _ctx(ctx), _timer(NULL)
{
	if (Timeout::enabled())
		enable();
}

Boost::BusTimeout::~BusTimeout()
{
	disable();
}

void Boost::BusTimeout::toggle()
{
	debug_log("boost: timeout %p toggled (%s)", this, Timeout::enabled() ? "on":"off");

	if (Timeout::enabled())
		enable();
	else
		disable();
}

void Boost::BusTimeout::enable()
{
	if (_timer)
		disable();

	_timer = new boost::asio::deadline_timer(*_ctx, boost::posix_time::milliseconds(Timeout::interval()));
	_timer->async_wait(boost::bind(&BusTimeout::callback, this, _1));
}

void Boost::BusTimeout::disable()
{
	if (!_timer)
		return;

	_timer->cancel();
	delete _timer;
	_timer = NULL;
}

void Boost::BusTimeout::callback(const boost::system::error_code &ec)
{
	// this error is a boost internal that is triggered when
	// cancel is called on the timer, just abort
	if (ec == boost::asio::error::operation_aborted)
		return;

	Timeout::handle();
	_timer->expires_from_now(boost::posix_time::milliseconds(Timeout::interval()));
	_timer->async_wait(boost::bind(&BusTimeout::callback, this, _1));
}

Boost::BusWatch::BusWatch(Watch::Internal *wi, boost::asio::io_service *ctx, BusDispatcher *dispatcher)
: Watch(wi), _ctx(ctx), _dispatcher(dispatcher), _wch(NULL)
{
	if (Watch::enabled())
		enable();
}

Boost::BusWatch::~BusWatch()
{
	disable();
}

void Boost::BusWatch::toggle()
{
	debug_log("boost: watch %p toggled (%s)", this, Watch::enabled() ? "on":"off");

	if (Watch::enabled())
		enable();
	else
		disable();
}

void Boost::BusWatch::enable()
{
	if (_wch)
		disable();

	_wch = new boost::asio::posix::stream_descriptor(*_ctx, ::dup(Watch::descriptor()));

	// there's currently no good detection in boost for HUP events
	if (Watch::flags() & (DBUS_WATCH_READABLE | DBUS_WATCH_ERROR))
		_wch->async_read_some(boost::asio::null_buffers(), boost::bind(&BusWatch::callback, this, _1, _2, Watch::flags() & (DBUS_WATCH_READABLE | DBUS_WATCH_ERROR)));
	if (Watch::flags() & DBUS_WATCH_WRITABLE)
		_wch->async_write_some(boost::asio::null_buffers(), boost::bind(&BusWatch::callback, this, _1, _2, DBUS_WATCH_WRITABLE));
}

void Boost::BusWatch::disable()
{
	if (!_wch)
		return;

	_wch->cancel();
	delete _wch;
	_wch = NULL;
}

void Boost::BusWatch::callback(const boost::system::error_code &ec, size_t, int flag)
{
	if (ec)
	{
		// this error is a boost internal that is triggered when
		// cancel is called on the watch, just abort
		if (ec == boost::asio::error::operation_aborted)
			return;
		if (flag & DBUS_WATCH_ERROR)
			Watch::handle(DBUS_WATCH_ERROR);
	}
	else
		Watch::handle(flag & ~DBUS_WATCH_ERROR);
	// boost implements a one-shot idiom, keep adding the callback back
	if (flag & (DBUS_WATCH_READABLE | DBUS_WATCH_ERROR))
	{
		_wch->async_read_some(boost::asio::null_buffers(), boost::bind(&BusWatch::callback, this, _1, _2, flag));
	}
	if (flag & DBUS_WATCH_WRITABLE)
	{
		_wch->async_write_some(boost::asio::null_buffers(), boost::bind(&BusWatch::callback, this, _1, _2, flag));
	}

	if (_dispatcher->has_something_to_dispatch())
		_dispatcher->dispatch_pending();
}

Boost::BusDispatcher::BusDispatcher()
: _ctx(NULL)
{
}

Boost::BusDispatcher::~BusDispatcher()
{
// TODO: Destroy allocated objects such as watches and timers
}

void Boost::BusDispatcher::attach(boost::asio::io_service *ctx)
{
	BOOST_ASSERT(_ctx == NULL); // just to be sane
	BOOST_ASSERT(ctx != NULL);

	_ctx = ctx;
}

Timeout *Boost::BusDispatcher::add_timeout(Timeout::Internal *wi)
{
	Timeout *t = new Boost::BusTimeout(wi, _ctx);

	debug_log("boost: added timeout %p (%s)", t, t->enabled() ? "on":"off");

	return t;
}

void Boost::BusDispatcher::rem_timeout(Timeout *t)
{
	debug_log("boost: removed timeout %p", t);

	delete t;
}

Watch *Boost::BusDispatcher::add_watch(Watch::Internal *wi)
{
	Watch *w = new Boost::BusWatch(wi, _ctx, this);

	debug_log("boost: added watch %p (%s) fd=%d flags=%d",
		w, w->enabled() ? "on":"off", w->descriptor(), w->flags()
	);
	return w;
}

void Boost::BusDispatcher::rem_watch(Watch *w)
{
	debug_log("boost: removed watch %p", w);

	delete w;
}
