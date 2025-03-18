#pragma once

#include <util/datetime/base.h>
#include <util/digest/numeric.h>

namespace NUdp {
    //! Struct representing UDP server options.
    struct TServerOptions {
        //! Port of the server
        ui32 Port = 161;

        //! Number of server's workers recieving packets.
        ui32 Workers = 5;

        //! Number of server's executors processing packets.
        //! If equals to zero callback is executed in server's
        //! workers loop. Should be used only if callback is
        //! really fast.
        ui32 Executors = 1;

        //! Maximum allowed size of server's queue for requests.
        //! If Exectutors equals is zero doesn't make sence.
        ui32 QueueSizeLimit = 200;

        //! Number of server's workers processing errors.
        //! Errors includes problems with recieving packets
        //! and main queue overflows.
        ui32 ErrorThreads = 1;

        //! Maximum allowed size of server's queue for errors.
        ui32 ErrorQueueSizeLimit = 20;

        // Signals and sockets poller timeout. Do not change it if
        // you do not know what you are doing.
        TDuration PollTimeout = TDuration::MilliSeconds(100);

        //! Sets Port and returns reference to this.
        /*!
         *  \param port Port value to set.
         */
        TServerOptions& SetPort(ui32 port);

        //! Sets Workers and returns reference to this.
        /*!
         *  \param workers Workers to set.
         */
        TServerOptions& SetWorkers(ui32 workers);

        //! Sets Executors and returns reference to this.
        /*!
         *  \param executors Executors to set.
         */
        TServerOptions& SetExecutors(ui32 executors);

        //! Sets QueueSizeLimit and returns reference to this.
        /*!
         *  \param queueSizeLimit QueueSizeLimit to set.
         */
        TServerOptions& SetQueueSizeLimit(ui32 queueSizeLimit);

        //! Sets ErrorThreads and returns reference to this.
        /*!
         *  \param queueSizeLimit QueueSizeLimit to set.
         */
        TServerOptions& SetErrorThreads(ui32 errorThreads);

        //! Sets ErrorQueueSizeLimit and returns reference to this.
        /*!
         *  \param errorQueueSizeLimit ErrorQueueSizeLimit to set.
         */
        TServerOptions& SetErrorQueueSizeLimit(ui32 errorQueueSizeLimit);

        //! Sets Poll Timeout and returns reference to this.
        /*!
         *  \param pollTimeout PollTimeout to set.
         */
        TServerOptions& SetPollTimeout(TDuration pollTimeout);
    };

}
