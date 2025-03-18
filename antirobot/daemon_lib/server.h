#pragma once

#include <library/cpp/http/server/http.h>

#include <util/generic/ptr.h>

namespace NAntiRobot {
    struct TCommandLineParams;

    class TServer {
    public:
        TServer(const TCommandLineParams& clParams);
        ~TServer();

        void Run();
    private:
        class TImpl;
        THolder<TImpl> Impl;
    };
}
