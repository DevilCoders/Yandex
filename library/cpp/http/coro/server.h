#pragma once

#include <util/network/address.h>
#include <functional>
#include <util/system/types.h>

class THttpInput;
class THttpOutput;
class TContExecutor;
class TCont;

namespace NCoroHttp {
    class THttpServer {
    public:
        // TODO - use http server options from library/cpp/http/server
        struct TConfig {
            TIpPort Port = 8080;
            TString Host;
            size_t CoroutineStackSize = 64000;
            size_t ListenQueueSize = 0;
            TContExecutor* Executor = nullptr;

            inline TConfig& SetHost(const TString& host) noexcept {
                Host = host;

                return *this;
            }

            inline TConfig& SetPort(TIpPort port) noexcept {
                Port = port;

                return *this;
            }

            inline TConfig& SetExecutor(TContExecutor* executor) noexcept {
                Executor = executor;

                return *this;
            }

            inline TConfig& SetStackSize(size_t ss) noexcept {
                CoroutineStackSize = ss;

                return *this;
            }

            inline TConfig& SetListenQueueSize(size_t n) noexcept {
                ListenQueueSize = n;

                return *this;
            }
        };

        struct TRequestContext {
            THttpInput* Input;
            THttpOutput* Output;
            const NAddr::IRemoteAddr* RemoteAddress;
            TCont* Cont;
        };

        struct TCallbacks {
            using TRequestCb = std::function<void(TRequestContext&)>;
            using TErrorCb = std::function<void()>;

            inline TCallbacks& SetRequestCb(const TRequestCb& cb) {
                OnRequestCb = cb;

                return *this;
            }

            inline TCallbacks& SetErrorCb(const TErrorCb& cb) {
                OnErrorCb = cb;

                return *this;
            }

            TRequestCb OnRequestCb;
            TErrorCb OnErrorCb;
        };

        THttpServer();
        ~THttpServer();

        // Stop server and close all opened sockets, async. RunCycle() will exit after that call
        void ShutDown();

        // Run server loop, sync
        void RunCycle(const TConfig& config, const TCallbacks& callbacks);

        // Run server loop, sync
        inline void RunCycle(const TCallbacks& callbacks) {
            RunCycle(TConfig(), callbacks);
        }

        // Get coroutine execution context
        TContExecutor* Executor() const noexcept;

        // True if server is up and running
        bool IsRunning() const noexcept;

    private:
        struct TImpl;
        THolder<TImpl> Impl_;
    };
}
