//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLED_RIPPLE_WEBSOCKET_WSDOORBASE_H
#define RIPPLED_RIPPLE_WEBSOCKET_WSDOORBASE_H

#include <ripple/basics/Log.h>
#include <ripple/websocket/WebSocket.h>
#include <beast/cxx14/memory.h> // <memory>
#include <beast/threads/Thread.h>

namespace ripple {
namespace websocket {

template <class WebSocket>
class Server
    : public beast::Stoppable
    , protected beast::Thread
{
private:
    // TODO: why is this recursive?
    using LockType = typename std::recursive_mutex;
    using ScopedLockType = typename std::lock_guard <LockType>;

    ServerDescription desc_;
    LockType m_endpointLock;
    typename WebSocket::EndpointPtr m_endpoint;

public:
    Server (ServerDescription const& desc)
       : beast::Stoppable (WebSocket::versionName(), desc.source)
        , Thread ("websocket")
        , desc_(desc)
    {
        startThread ();
    }

    ~Server ()
    {
        stopThread ();
    }

private:
    void run () override
    {
        WriteLog (lsWARNING, WebSocket)
                << "Websocket: '" << desc_.port.name
                << "' creating endpoint " << desc_.port.ip.to_string()
                << ":" << std::to_string(desc_.port.port)
                << (desc_.port.allow_admin ? "(Admin)" : "");

        auto handler = WebSocket::makeHandler (desc_);
        {
            ScopedLockType lock (m_endpointLock);
            m_endpoint = WebSocket::makeEndpoint (std::move (handler));
        }

        WriteLog (lsWARNING, WebSocket)
                << "Websocket: '" << desc_.port.name
                << "' listening on " << desc_.port.ip.to_string()
                << ":" << std::to_string(desc_.port.port)
                << (desc_.port.allow_admin ? "(Admin)" : "");

        listen();
        {
            ScopedLockType lock (m_endpointLock);
            m_endpoint.reset();
        }

        WriteLog (lsWARNING, WebSocket)
                << "Websocket: '" << desc_.port.name
                << "' finished listening on " << desc_.port.ip.to_string()
                << ":" << std::to_string(desc_.port.port)
                << (desc_.port.allow_admin ? "(Admin)" : "");

        stopped ();
        WriteLog (lsWARNING, WebSocket)
                << "Websocket: '" << desc_.port.name
                << "' stopped on " << desc_.port.ip.to_string()
                << ":" << std::to_string(desc_.port.port)
                << (desc_.port.allow_admin ? "(Admin)" : "");
    }

    void onStop () override
    {
        WriteLog (lsWARNING, WebSocket)
                << "Websocket: '" << desc_.port.name
                << "' onStop " << desc_.port.ip.to_string()
                << ":" << std::to_string(desc_.port.port)
                << (desc_.port.allow_admin ? "(Admin)" : "");

        typename WebSocket::EndpointPtr endpoint;
        {
            ScopedLockType lock (m_endpointLock);
            endpoint = m_endpoint;
        }

        if (endpoint)
            endpoint->stop ();
        signalThreadShouldExit ();
    }

    void listen();
};

} // websocket
} // ripple

#endif
