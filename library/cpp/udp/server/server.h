#pragma once

#include "callbacks.h"
#include "options.h"
#include "packet.h"

#include <functional>

namespace NUdp {
    // Class representing UDP server.
    class TUdpServer {
    public:
        /*! TUdpServer constructor.
         *  \param options Server options.
         *  \param callback Callback for packets handling.
         */
        TUdpServer(const TServerOptions& options,
                   TCallback callback);

        ~TUdpServer();

        //! Starts the server in separate thread.
        //! Throws if start fails.
        void Start();

        //! Wait for server shut down.
        //! Throws if server fails.
        void Wait();

        //! Gracefully stops the server.
        //! Throws if stop fails.
        void Stop();

        // Sets the error callback.
        /*!
         *  \param callback Callback to set.
         */
        void SetErrorCallback(TErrorCallback callback);

        // Sets the queue overflow callback.
        /*!
         *  \param callback Queue overflow callback to set.
         */
        void SetQueueOverflowCallback(TQueueOverflowCallback callback);

    private:
        class TImpl;
        THolder<TImpl> Impl_;
    };

}
