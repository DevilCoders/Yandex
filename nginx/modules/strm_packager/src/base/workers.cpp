#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/base/log_levels.h>

#include <util/string/printf.h>

namespace NStrm::NPackager {
    class TBufferSubrequestWorker: public ISubrequestWorker {
    public:
        TBufferSubrequestWorker(int expectedHttpCode);

        NThreading::TFuture<TBuffer> GetFuture();

    private:
        void AcceptHeaders(const THeadersOut& headers) override;
        void AcceptData(char const* const begin, char const* const end) override;
        void SubrequestFinished(const TFinishStatus status) override;

        const int ExpectedHttpCode;
        NThreading::TPromise<TBuffer> Promise;
        TBuffer Data;
    };

    class TBlobSubrequestWorker: public ISubrequestWorker {
    public:
        TBlobSubrequestWorker(int expectedHttpCode, ui8* resultBegin, ui8* resultEnd, bool requireEqualSize);

        NThreading::TFuture<TSimpleBlob> GetFuture();

    private:
        void AcceptHeaders(const THeadersOut& headers) override;
        void AcceptData(char const* const begin, char const* const end) override;
        void SubrequestFinished(const TFinishStatus status) override;

        const int ExpectedHttpCode;
        NThreading::TPromise<TSimpleBlob> Promise;
        ui8* const ResultBegin;
        ui8* ResultPos;
        ui8* const ResultEnd;
        const bool RequireEqualSize;
    };

    TRequestWorker::TRequestWorker(TRequestContext& context, const TLocationConfig& config, const bool kalturaMode, TString logPrefix)
        : Config(config)
        , KalturaMode(kalturaMode)
        , MemoryPool(1024, TMemoryPool::TExpGrow::Instance())
        , CreationTime(TInstant::Now())
        , Context(context)
        , LogPrefix(std::move(logPrefix))
        , PendingChain(nullptr)
        , PendingChainLast(nullptr)
        , ChunkedTransferEncodingDisabled(false)
        , ContentLength(0)
    {
    }

    TRequestWorker::~TRequestWorker() {
        // nothing
    }

    void TRequestWorker::AddCustomHeader(const TStringBuf key, const TStringBuf value) {
        Y_ENSURE(!Context.Request->header_sent);
        ngx_table_elt_t* const header = (ngx_table_elt_t*)ngx_list_push(&Context.Request->headers_out.headers);
        Y_ENSURE(header);
        header->value = NginxString(value, Context.Request->pool);
        header->key = NginxString(key, Context.Request->pool);
        header->lowcase_key = NginxString(key, Context.Request->pool, /*makeLowercase = */ true).data;
        header->hash = ngx_hash_key(header->lowcase_key, key.length());
    }

    void TRequestWorker::AddChainToPending(ngx_chain_t* chain) {
        Y_ENSURE(chain && !chain->next && chain->buf);
        if (PendingChain) {
            Y_ENSURE(PendingChainLast);
            PendingChainLast->next = chain;
            PendingChainLast = chain;
        } else {
            PendingChain = chain;
            PendingChainLast = chain;
        }
        ContentLength += chain->buf->last - chain->buf->pos;
    }

    void TRequestWorker::DisableChunkedTransferEncoding() {
        ChunkedTransferEncodingDisabled = true;
        Context.Request->allow_ranges = 1;
    }

    void TRequestWorker::SendPendingChain() {
        Y_ENSURE(PendingChain);
        ngx_chain_t* out = PendingChain;
        PendingChain = nullptr;
        PendingChainLast = nullptr;

        if (!Context.Request->header_sent) {
            Context.Request->headers_out.content_length_n = ChunkedTransferEncodingDisabled ? PromisedContentLength.GetOrElse(ContentLength) : -1;
            Context.Request->headers_out.status = NGX_HTTP_OK;
            const int rc = ngx_http_send_header(Context.Request);
            Y_ENSURE_EX(rc == NGX_OK || rc == NGX_AGAIN, THttpError(rc, TLOG_WARNING, /*ngxSendError = */ true) << "ngx_http_send_header rc = " << rc);
        }

        for (ngx_chain_t* cn = out; cn; cn = cn->next) {
            if (cn->buf->last_buf) {
                Context.Finished = true;
            }
        }

        const int rc = ngx_http_output_filter(Context.Request, out);
        Y_ENSURE_EX(rc == NGX_OK || rc == NGX_AGAIN, THttpError(rc, TLOG_WARNING, /*ngxSendError = */ true) << "ngx_http_output_filter rc = " << rc);

        if (Context.Finished) {
            Context.StartFinalize();
        }
    }

    void TRequestWorker::SendData(const TSendData data) {
        Y_ENSURE(Context.Worker == this);
        Y_ENSURE(!Context.Finished);

        if (data.ContentLengthPromise.Defined()) {
            Y_ENSURE(!PromisedContentLength.Defined());
            Y_ENSURE(ContentLength == 0);
            PromisedContentLength = data.ContentLengthPromise;
        }

        if (data.Blob.empty()) {
            Y_ENSURE(data.Flush);
            Flush();
            return;
        }

        ngx_chain_t* out = MemoryPool.AllocateZeroArray<ngx_chain_t>(1);
        out->buf = MemoryPool.AllocateZeroArray<ngx_buf_t>(1);

        out->buf->memory = 1;
        out->buf->flush = data.Flush ? 1 : 0;
        out->buf->pos = (ui8*)data.Blob.begin();
        out->buf->last = (ui8*)data.Blob.end();

        AddChainToPending(out);
        if (data.Flush && (PromisedContentLength.Defined() || !ChunkedTransferEncodingDisabled)) {
            SendPendingChain();
        }
    }

    void TRequestWorker::Flush() {
        Y_ENSURE(Context.Worker == this);
        Y_ENSURE(!Context.Finished);

        if (PromisedContentLength.Defined() || !ChunkedTransferEncodingDisabled) {
            AddChainToPending(Context.FlushChain);
            SendPendingChain();
        }
    }

    void TRequestWorker::Finish() {
        // fail if finish before start, or more than once
        Y_ENSURE(Context.Worker == this);
        Y_ENSURE(!Context.Finished);
        Y_ENSURE(Context.Request->buffered & TRequestContext::BufferedFlag);

        AddChainToPending(Context.FinishChain);
        SendPendingChain();

        Y_ENSURE(PromisedContentLength.GetOrElse(ContentLength) == ContentLength);
    }

    void TRequestWorker::SetContentType(const TStringBuf type) {
        Context.Request->headers_out.content_type = NginxString(type, Context.Request->pool);
    }

    TNgxAllocator<char> TRequestWorker::GetAllocator() {
        return TNgxAllocator<char>(Context.Request->pool);
    }

    TNgxTimer* TRequestWorker::MakeTimer() {
        return GetPoolUtil<TNgxTimer>().New(Context.Request);
    }

    TNgxLogger TRequestWorker::GetLogger(ELogPriority level) {
        return TNgxLogger(ToNgxLogLevel(level), Context.Request->connection->log);
    }

    TNgxStreamLogger TRequestWorker::LogEmerg() {
        return std::move(GetLogger(TLOG_EMERG).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogAlert() {
        return std::move(GetLogger(TLOG_ALERT).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogCrit() {
        return std::move(GetLogger(TLOG_CRIT).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogError() {
        return std::move(GetLogger(TLOG_ERR).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogWarn() {
        return std::move(GetLogger(TLOG_WARNING).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogNotice() {
        return std::move(GetLogger(TLOG_NOTICE).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogInfo() {
        return std::move(GetLogger(TLOG_INFO).Stream() << LogPrefix << " ");
    }

    TNgxStreamLogger TRequestWorker::LogDebug() {
        return std::move(GetLogger(TLOG_DEBUG).Stream() << LogPrefix << " ");
    }

    void TRequestWorker::LogException(char const* const note, const std::exception_ptr e, const ELogPriority logLevelLimit) {
        try {
            std::rethrow_exception(e);
        } catch (const THttpError& e) {
            ngx_log_error(ToNgxLogLevel(Max(logLevelLimit, e.LogPriority)), Context.Request->connection->log, 0, "%s %s c++ exception [[ %s ]] ", LogPrefix.c_str(), note ? note : "", e.what());
        } catch (const std::exception& e) {
            ngx_log_error(ToNgxLogLevel(logLevelLimit), Context.Request->connection->log, 0, "%s %s c++ exception [[ %s ]] ", LogPrefix.c_str(), note ? note : "", e.what());
        } catch (...) {
            ngx_log_error(ToNgxLogLevel(logLevelLimit), Context.Request->connection->log, 0, "%s %s c++ unknown exception", LogPrefix.c_str(), note ? note : "");
        }
    }

    void TRequestWorker::CreatePendingSubrequests() {
        if (Context.Finished || Context.Aborting) {
            PendindSubrequests.clear();
            return;
        }

        for (const auto& [params, subrequestWorker] : PendindSubrequests) {
            CreateSubrequestImpl(params, subrequestWorker);
        }
        PendindSubrequests.clear();
    }

    void TRequestWorker::CreateSubrequest(const TSubrequestParams& params, ISubrequestWorker* const subrequestWorker) {
        PendindSubrequests.emplace_back(params, subrequestWorker);
    }

    void TRequestWorker::CreateSubrequestImpl(const TSubrequestParams& params, ISubrequestWorker* const subrequestWorker) {
        Y_ENSURE(params.Uri);
        Y_ENSURE(subrequestWorker);

        ngx_str_t ngx_uri = NginxString(params.Uri, Context.Request->pool);
        ngx_str_t ngx_args = NginxString(params.Args, Context.Request->pool);

        ngx_http_post_subrequest_t* postSubrequest = TNgxPoolUtil<ngx_http_post_subrequest_t>::Calloc(Context.Request->pool);
        postSubrequest->handler = TRequestContext::SubrequestFinishedCallback;
        postSubrequest->data = GetPoolUtil<TRequestContext::TPostSubrequestData>().New(*subrequestWorker);

        ngx_http_request_t* subrequest = nullptr;

        Y_ENSURE(NGX_OK == ngx_http_subrequest(
                               Context.Request,
                               &ngx_uri,
                               params.Args ? &ngx_args : nullptr,
                               &subrequest,
                               postSubrequest,
                               params.Background ? NGX_HTTP_SUBREQUEST_BACKGROUND : 0));

        Y_ENSURE(subrequest);
        Y_ENSURE(subrequest->parent == Context.Request);
        Y_ENSURE(subrequest->main == Context.Request);
        ++Context.RunningSubrequests;

        subrequest->subrequest_get_special_response = 1;

        // reset subrequest headers
        memset(&subrequest->headers_in, 0, sizeof(subrequest->headers_in));

        Y_ENSURE(
            ngx_list_init(
                &subrequest->headers_in.headers,
                subrequest->pool,
                Context.Request->headers_in.headers.nalloc,
                sizeof(ngx_table_elt_t)) == NGX_OK);

        constexpr auto computeHash = [](const TStringBuf& s) -> ngx_uint_t {
            ngx_uint_t result = 0;
            for (size_t i = 0; i < s.Size(); ++i) {
                result = ngx_hash(result, s[i]);
            }
            return result;
        };

        static constexpr TStringBuf accept("Accept");
        static constexpr TStringBuf acceptLc("accept");
        static constexpr ngx_uint_t acceptHash = computeHash(acceptLc);

        static constexpr TStringBuf range("Range");
        static constexpr TStringBuf rangeLc("range");
        static constexpr ngx_uint_t rangeHash = computeHash(rangeLc);

        static constexpr TStringBuf ifrangeLc("if-range");
        static constexpr ngx_uint_t ifrangeHash = computeHash(ifrangeLc);

        static constexpr TStringBuf hostLc("host");
        static constexpr ngx_uint_t hostHash = computeHash(hostLc);

        ngx_http_core_main_conf_t& coreMainConf = *(ngx_http_core_main_conf_t*)ngx_http_get_module_main_conf(Context.Request, ngx_http_core_module);

        const auto addHeader = [subrequest, &coreMainConf](const TStringBuf& key, const TStringBuf& keyLc, ngx_uint_t hash, ngx_str_t value) {
            ngx_table_elt_t* const elt = (ngx_table_elt_t*)ngx_list_push(&subrequest->headers_in.headers);
            Y_ENSURE(elt);
            Y_ENSURE(key.Size() == keyLc.Size());

            elt->key = ngx_str_t{.len = key.Size(), .data = (u_char*)key.Data()};
            elt->lowcase_key = (u_char*)keyLc.Data();
            elt->value = value;
            elt->hash = hash;

            if (hash == hostHash && keyLc == hostLc) {
                // host handler require subrequest->http_connection that is not set
                subrequest->headers_in.host = elt;
                return;
            }

            ngx_http_header_t const* const hh = (ngx_http_header_t*)ngx_hash_find(&coreMainConf.headers_in_hash, hash, (u_char*)keyLc.Data(), keyLc.Size());

            if (hh) {
                Y_ENSURE(hh->handler(subrequest, elt, hh->offset) == NGX_OK);
            }
        };

        // copy headers from original request
        ngx_uint_t origHeadersPartIndex = 0;
        ngx_list_part_t* origHeadersPart = &Context.Request->headers_in.headers.part;
        for (;; ++origHeadersPartIndex) {
            if (origHeadersPartIndex == origHeadersPart->nelts) {
                origHeadersPartIndex = 0;
                origHeadersPart = origHeadersPart->next;
            }
            if (!origHeadersPart) {
                break;
            }

            const ngx_table_elt_t& origHeader = *(((ngx_table_elt_t*)origHeadersPart->elts) + origHeadersPartIndex);

            const TStringBuf origHeaderKey((char*)origHeader.key.data, origHeader.key.len);
            const TStringBuf origHeaderLcKey((char*)origHeader.lowcase_key, origHeader.key.len);

            if ((origHeader.hash == rangeHash && origHeaderLcKey == rangeLc) ||
                (origHeader.hash == ifrangeHash && origHeaderLcKey == ifrangeLc) ||
                (origHeader.hash == acceptHash && origHeaderLcKey == acceptLc))
            {
                continue;
            }

            if (Config.SubrequestHeadersDrop.Defined() && Config.SubrequestHeadersDrop->find(origHeaderLcKey) != Config.SubrequestHeadersDrop->end()) {
                continue;
            }

            addHeader(origHeaderKey, origHeaderLcKey, origHeader.hash, origHeader.value);
        }

        addHeader(accept, acceptLc, acceptHash, ngx_string("*/*"));

        if (params.RangeBegin && params.RangeEnd) {
            addHeader(range, rangeLc, rangeHash, NginxString(Sprintf("bytes=%lu-%lu", *params.RangeBegin, *params.RangeEnd - 1), subrequest->pool));
            subrequest->subrequest_ranges = 1;
            subrequest->allow_ranges = 1;
        }

        const size_t varialbesCount = ((ngx_http_core_main_conf_t*)ngx_http_get_module_main_conf(subrequest, ngx_http_core_module))->variables.nelts;
        if (Config.SubrequestClearVariables.GetOrElse(false)) {
            // clear subrequest variables
            subrequest->variables = TNgxPoolUtil<ngx_http_variable_value_t>::Calloc(subrequest->pool, varialbesCount);
        } else {
            // copy subrequest variables
            subrequest->variables = TNgxPoolUtil<ngx_http_variable_value_t>::Alloc(subrequest->pool, varialbesCount);
            memcpy(subrequest->variables, Context.Request->variables, varialbesCount * sizeof(ngx_http_variable_value_t));
        }
    }

    NThreading::TFuture<TBuffer> TRequestWorker::CreateSubrequest(
        const TSubrequestParams& params,
        int expectedHttpCode) {
        TBufferSubrequestWorker* srWorker = GetPoolUtil<TBufferSubrequestWorker>().New(expectedHttpCode);
        CreateSubrequest(
            params,
            srWorker);
        return srWorker->GetFuture();
    }

    NThreading::TFuture<TSimpleBlob> TRequestWorker::CreateSubrequest(
        const TSubrequestParams& params,
        int expectedHttpCode,
        ui8* resultBegin,
        ui8* resultEnd,
        bool requireEqualSize) {
        Y_ENSURE(resultEnd > resultBegin);
        TBlobSubrequestWorker* srWorker = GetPoolUtil<TBlobSubrequestWorker>().New(expectedHttpCode, resultBegin, resultEnd, requireEqualSize);
        CreateSubrequest(
            params,
            srWorker);
        return srWorker->GetFuture();
    }

    TStringBuf TRequestWorker::GetUri() const {
        return TStringBuf((char*)Context.Request->uri.data, Context.Request->uri.len);
    }

    TStringBuf TRequestWorker::GetArgs() const {
        return TStringBuf((char*)Context.Request->args.data, Context.Request->args.len);
    }

    TStringBuf TRequestWorker::GetComplexValue(TComplexValue cv) const {
        ngx_str_t value;
        auto res = ngx_http_complex_value(Context.Request, &cv, &value);
        Y_ENSURE(res == NGX_OK, "failed to get complex value: " << TString((char*)cv.value.data, cv.value.len));
        return {(char*)value.data, value.len};
    }

    TMaybe<TStringBuf> TRequestWorker::GetArg(const TStringBuf arg, const bool required) const {
        ngx_str_t value;
        const ngx_int_t ret = ngx_http_arg(Context.Request, (u_char*)arg.data(), arg.size(), &value);
        if (ret == NGX_OK) {
            return TStringBuf((char*)value.data, value.len);
        } else {
            Y_ENSURE_EX(!required, THttpError(NGX_HTTP_BAD_REQUEST, TLOG_ERR) << "arg '" << arg << "' is required, but not set");
            return {};
        }
    }

    TVariables& TRequestWorker::GetVariables() {
        return Context.Variables;
    }

    TMetrics& TRequestWorker::GetMetrics() {
        return Context.Variables.Metrics;
    }

    TBufferSubrequestWorker::TBufferSubrequestWorker(int expectedHttpCode)
        : ExpectedHttpCode(expectedHttpCode)
        , Promise(NThreading::NewPromise<TBuffer>())
    {
    }

    void TBufferSubrequestWorker::AcceptHeaders(const THeadersOut& headers) {
        try {
            Y_ENSURE_EX(ExpectedHttpCode == headers.Status(), THttpError(ProxyBadUpstreamHttpCode(headers.Status()), TLOG_WARNING) << "subrequest http code: " << headers.Status() << " but expected " << ExpectedHttpCode);
            if (headers.ContentLength().Defined()) {
                Data.Reserve(*headers.ContentLength());
            }
        } catch (const TFatalExceptionContainer&) {
            throw;
        } catch (...) {
            Promise.SetException(std::current_exception());
        }
    }

    void TBufferSubrequestWorker::AcceptData(char const* const begin, char const* const end) {
        if (Promise.HasException()) {
            return;
        }
        Data.Append(begin, end);
    }

    void TBufferSubrequestWorker::SubrequestFinished(const TFinishStatus status) {
        if (Promise.HasException()) {
            return;
        }
        try {
            CheckFinished(status);
        } catch (...) {
            Promise.SetException(std::current_exception());
            return;
        }
        Promise.SetValue(std::move(Data));
    }

    NThreading::TFuture<TBuffer> TBufferSubrequestWorker::GetFuture() {
        return Promise.GetFuture();
    }

    TBlobSubrequestWorker::TBlobSubrequestWorker(int expectedHttpCode, ui8* resultBegin, ui8* resultEnd, bool requireEqualSize)
        : ExpectedHttpCode(expectedHttpCode)
        , Promise(NThreading::NewPromise<TSimpleBlob>())
        , ResultBegin(resultBegin)
        , ResultPos(resultBegin)
        , ResultEnd(resultEnd)
        , RequireEqualSize(requireEqualSize)
    {
    }

    void TBlobSubrequestWorker::AcceptHeaders(const THeadersOut& headers) {
        try {
            Y_ENSURE_EX(ExpectedHttpCode == headers.Status(), THttpError(ProxyBadUpstreamHttpCode(headers.Status()), TLOG_WARNING) << "subrequest http code: " << headers.Status() << " but expected " << ExpectedHttpCode);
            const auto contentLength = headers.ContentLength();
            if (contentLength.Defined()) {
                if (RequireEqualSize) {
                    Y_ENSURE_EX(size_t(ResultEnd - ResultBegin) == *contentLength, THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_ERR) << "subrequest content length: " << *contentLength << " but expected " << (ResultEnd - ResultBegin));
                } else {
                    Y_ENSURE_EX(size_t(ResultEnd - ResultBegin) >= *contentLength, THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_ERR) << "subrequest content length: " << *contentLength << " but expected at most " << (ResultEnd - ResultBegin));
                }
            }
        } catch (const TFatalExceptionContainer&) {
            throw;
        } catch (...) {
            Promise.SetException(std::current_exception());
        }
    }

    void TBlobSubrequestWorker::AcceptData(char const* const begin, char const* const end) {
        if (Promise.HasException()) {
            return;
        }
        try {
            Y_ENSURE_EX(ResultEnd - ResultPos >= end - begin,
                        THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_ERR)
                            << "subrequest data exceeded expected size: already have " << (ResultPos - ResultBegin)
                            << " now got another " << (end - begin) << " but expected at most " << (ResultEnd - ResultBegin));

            std::memcpy(ResultPos, begin, end - begin);
            ResultPos += end - begin;
        } catch (const TFatalExceptionContainer&) {
            throw;
        } catch (...) {
            Promise.SetException(std::current_exception());
        }
    }

    void TBlobSubrequestWorker::SubrequestFinished(const TFinishStatus status) {
        if (Promise.HasException()) {
            return;
        }
        try {
            CheckFinished(status);
            Y_ENSURE_EX(ResultPos == ResultEnd || !RequireEqualSize, THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_ERR) << "subrequest got " << (ResultPos - ResultBegin) << " bytes, but expected " << (ResultEnd - ResultBegin));
        } catch (...) {
            Promise.SetException(std::current_exception());
            return;
        }
        Promise.SetValue(TSimpleBlob(ResultBegin, ResultPos - ResultBegin));
    }

    NThreading::TFuture<TSimpleBlob> TBlobSubrequestWorker::GetFuture() {
        return Promise.GetFuture();
    }

}
