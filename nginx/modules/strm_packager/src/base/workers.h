#pragma once

#include <nginx/modules/strm_packager/src/base/allocator.h>
#include <nginx/modules/strm_packager/src/base/context.h>
#include <nginx/modules/strm_packager/src/base/headers.h>
#include <nginx/modules/strm_packager/src/base/http_error.h>
#include <nginx/modules/strm_packager/src/base/logger.h>
#include <nginx/modules/strm_packager/src/base/pool_util.h>
#include <nginx/modules/strm_packager/src/base/types.h>
#include <nginx/modules/strm_packager/src/base/fatal_exception.h>
#include <nginx/modules/strm_packager/src/common/future_utils.h>

#include <strm/media/formats/unified/types.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/string/cast.h>

#include <library/cpp/logger/priority.h>

#include <utility>

namespace NStrm::NPackager {
    class TLocationConfig;
    class TNgxTimer;

    using TSimpleBlob = TArrayRef<const ui8>;

    class ISubrequestWorker {
    public:
        struct TFinishStatus {
            ngx_int_t Code;
            bool GotLastInChain;
        };

    public:
        ISubrequestWorker() = default;
        virtual ~ISubrequestWorker() = default;

        virtual void AcceptHeaders(const THeadersOut& headers) = 0;
        virtual void AcceptData(char const* const begin, char const* const end) = 0;
        virtual void SubrequestFinished(const TFinishStatus status) = 0;

    protected:
        static inline void CheckFinished(const TFinishStatus status) {
            Y_ENSURE_EX(status.Code == NGX_OK, THttpError(status.Code, TLOG_WARNING) << "subrequest non-zero error code: " << status.Code);
            Y_ENSURE_EX(status.GotLastInChain, THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_WARNING) << "subrequest did not get last_in_chain");
        }
    };

    struct TSubrequestParams {
        TString Uri;
        TString Args;
        TMaybe<ui64> RangeBegin;
        TMaybe<ui64> RangeEnd;
        bool Background = false;
    };

    struct TSendData {
        TSimpleBlob Blob;
        bool Flush;
        TMaybe<ui64> ContentLengthPromise = {};
    };

    class TRequestWorker {
    public:
        TRequestWorker(TRequestContext& context, const TLocationConfig& config, const bool kalturaMode, TString logPrefix);
        virtual ~TRequestWorker();

        TStringBuf GetUri() const;
        TStringBuf GetArgs() const;
        TMaybe<TStringBuf> GetArg(const TStringBuf arg, const bool required) const;

        template <typename T>
        TMaybe<T> GetArg(const TStringBuf arg, const bool required) const;

        TStringBuf GetComplexValue(TComplexValue val) const;

        template <typename T>
        T GetComplexValue(TComplexValue val) const;

        TVariables& GetVariables();
        TMetrics& GetMetrics();

        TNgxAllocator<char> GetAllocator();

        TNgxTimer* MakeTimer();

        template <typename T>
        TNgxPoolUtil<T> GetPoolUtil();

        TNgxLogger GetLogger(const ELogPriority level);

        TNgxStreamLogger LogEmerg();
        TNgxStreamLogger LogAlert();
        TNgxStreamLogger LogCrit();
        TNgxStreamLogger LogError();
        TNgxStreamLogger LogWarn();
        TNgxStreamLogger LogNotice();
        TNgxStreamLogger LogInfo();
        TNgxStreamLogger LogDebug();

        void LogException(char const* const note, const std::exception_ptr e, const ELogPriority logLevelLimit = TLOG_WARNING);

        template <typename T>
        TSimpleBlob HangDataInPool(T data);

        template <typename TFunc>
        auto MakeIndependentCallback(TFunc&& callback);

        void AddCustomHeader(const TStringBuf key, const TStringBuf value);
        void SetContentType(const TStringBuf type);
        void SendData(const TSendData data);
        void Flush();
        void Finish();
        void DisableChunkedTransferEncoding();

        void CreateSubrequest(const TSubrequestParams& params, ISubrequestWorker* const subrequestWorker);

        NThreading::TFuture<TBuffer> CreateSubrequest(
            const TSubrequestParams& params,
            int expectedHttpCode);

        NThreading::TFuture<TSimpleBlob> CreateSubrequest(
            const TSubrequestParams& params,
            int expectedHttpCode,
            ui8* resultBegin,
            ui8* resultEnd,
            bool requireEqualSize);

        template <typename F>
        auto MakeFatalOnException(F&& func);

        virtual void Work() = 0;

    public:
        const TLocationConfig& Config;
        const bool KalturaMode;

        TMemoryPool MemoryPool;
        const TInstant CreationTime;

    private:
        void AddChainToPending(ngx_chain_t*);
        void SendPendingChain();

        void CreatePendingSubrequests();
        void CreateSubrequestImpl(const TSubrequestParams& params, ISubrequestWorker* const subrequestWorker);

        TRequestContext& Context;

        const TString LogPrefix;

        ngx_chain_t* PendingChain;
        ngx_chain_t* PendingChainLast;

        bool ChunkedTransferEncodingDisabled;
        ui64 ContentLength;
        TMaybe<ui64> PromisedContentLength;

        TVector<std::pair<TSubrequestParams, ISubrequestWorker*>> PendindSubrequests;

        friend class TNgxTimer;
        friend class TRequestContext;
    };

    template <typename T>
    TMaybe<T> TRequestWorker::GetArg(const TStringBuf arg, const bool required) const {
        const TMaybe<TStringBuf> strValue = GetArg(arg, required);
        if (strValue.Defined()) {
            try {
                return FromString<T>(strValue->data(), strValue->size());
            } catch (const yexception& ye) {
                ythrow THttpError(NGX_HTTP_BAD_REQUEST, TLOG_ERR, ye) << "cant parse arg '" << arg << "' value";
            }
        } else {
            return {};
        }
    }

    template <typename T>
    TNgxPoolUtil<T> TRequestWorker::GetPoolUtil() {
        return TNgxPoolUtil<T>(Context.Request->pool);
    }

    namespace NImpl {
        template <typename F>
        class TFatalOnExceptionFunctor {
        public:
            TFatalOnExceptionFunctor(TRequestWorker& request, F&& func)
                : Request(request)
                , Func(std::move(func))
            {
            }

            template <typename... Args>
            void operator()(Args... args) {
                try {
                    Func(args...);
                } catch (const TFatalExceptionContainer&) {
                    throw;
                } catch (...) {
                    const std::exception_ptr exception = std::current_exception();
                    Request.LogException("TFatalOnExceptionFunctor: ", exception);
                    throw TFatalExceptionContainer(exception);
                }
            }

        private:
            TRequestWorker& Request;
            F Func;
        };
    }

    template <typename F>
    auto TRequestWorker::MakeFatalOnException(F&& func) {
        return NImpl::TFatalOnExceptionFunctor(*this, std::move(func));
    }

    namespace NDetails {
        template <typename T>
        inline ui64 GetSize(const T& data) {
            return data.Size();
        }

        template <typename T>
        inline ui8 const* GetData(const T& data) {
            return (ui8*)data.Data();
        }

        inline ui8 const* GetData(const TBytesChunk& data) {
            return (ui8 const*)data.View().data();
        }

        inline ui64 GetSize(const TBytesVector& data) {
            return data.size();
        }

        inline ui8 const* GetData(const TBytesVector& data) {
            return (ui8 const*)data.data();
        }
    }

    template <typename T>
    TSimpleBlob TRequestWorker::HangDataInPool(T dataIn) {
        Y_ENSURE(NDetails::GetSize(dataIn) > 0);
        T* data = GetPoolUtil<T>().New(std::move(dataIn));
        return TSimpleBlob(NDetails::GetData(*data), NDetails::GetSize(*data));
    }

    template <typename TFunc>
    auto TRequestWorker::MakeIndependentCallback(TFunc&& callback) {
        return [this, callback]<typename... TArgs>(TArgs... args) mutable {
            try {
                if (this->Context.Aborting) {
                    return;
                }

                ngx_http_set_log_request(this->Context.Request->connection->log, this->Context.Request);

                callback(std::forward<TArgs>(args)...);

                this->CreatePendingSubrequests();

                ngx_http_run_posted_requests(this->Context.Request->connection);

            } catch (const TFatalExceptionContainer& fec) {
                TRequestContext::ExceptionLogAndFinalizeRequest(fec.Exception(), this->Context.Request);
            } catch (...) {
                TRequestContext::ExceptionLogAndFinalizeRequest(std::current_exception(), this->Context.Request);
            }
        };
    }

    template <typename T>
    T TRequestWorker::GetComplexValue(TComplexValue val) const {
        const TStringBuf s = GetComplexValue(val);
        try {
            return FromString<T>(s);
        } catch (const yexception& ye) {
            ythrow THttpError(NGX_HTTP_BAD_REQUEST, TLOG_ERR, ye) << "cant parse complex value '" << s << "'";
        }
    }

}
