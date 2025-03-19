#pragma once

namespace NController {
    enum TServerStatusByController {
        ssbcStarting    /* "Starting" */,
        ssbcActive      /* "Active" */,
        ssbcNotRunnable /* "NotRunnable" */,
        ssbcStopped     /* "Stopped" */,
        ssbcStopping    /* "Stopping" */,
    };
}
