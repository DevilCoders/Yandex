#include "cbb.h"

#include "cbb_cache.h"
#include "cbb_id.h"
#include "config_global.h"
#include "eventlog_err.h"
#include "neh_requesters.h"
#include "parse_cbb_response.h"
#include "unified_agent_log.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/http_helpers.h>

#include <library/cpp/threading/future/async.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/function.h>
#include <util/generic/ptr.h>
#include <util/network/hostip.h>
#include <util/stream/tempbuf.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>
#include <util/thread/pool.h>

#include <string_view>

using namespace NAntirobotEvClass;
using namespace NThreading;

namespace NAntiRobot {
    namespace {
        template <typename Func>
        TFuture<TFutureType<TFunctionResult<Func>>> SafeAsync(Func&& func, IThreadPool& queue) {
            try {
                return NThreading::Async(func, queue);
            } catch (const yexception& ex) {
                EVLOG_MSG << EVLOG_ERROR << ex.what();
            }

            TPromise<TFunctionResult<Func>> error;
            error.SetException(TString());
            return error.GetFuture();
        }

        const TString CBB_API_PREFIX = "/cgi-bin/";
        constexpr std::array<TStringBuf, 2> CBB_CACHE_PATHS {
            "/cgi-bin/get_range.pl"_sb,
            "/cgi-bin/check_flag.pl"_sb,
        };
        TStringBuf OK = "Ok";

        inline TString DoCbbRequest(const THttpRequest& req, const TCbbIO::TOptions& options) {
            const auto cbbResult = FetchHttpDataUnsafe(&TCbbNehRequester::Instance(), options.CbbApiHost, req, options.Timeout, "http");
            if (options.Cache && IsIn(CBB_CACHE_PATHS, req.GetPath())) {
                options.Cache->Set(req.GetPathAndQuery(), cbbResult);
            }

            return cbbResult;
        }
    }

    class TCbbApiError : public yexception {
    };

    bool ReadCbbStringList(
        TCbbGroupId flag,
        TVector<TString>& list,
        const TCbbIO::TOptions& options,
        const TString& tvmTicket,
        const TString& format,
        TStringBuf title
    ) {
        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "get_range.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag", ToString(flag))
               .AddCgiParam("with_format", format);

            NParseCbb::ParseTextList(list, DoCbbRequest(req, options));

            return true;
        } catch (...) {
            EVLOG_MSG << "CBB error on getting " << title
                        << ", flag: " << flag
                        << ", exception: " << CurrentExceptionMessage();
        }
        return false;
    }

    bool RemoveCbbTxtBlock(TCbbGroupId cbbFlag, const TString& text, const TCbbIO::TOptions& options, const TString& tvmTicket) {
        TString cbbRes;
        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "set_range.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag",        ToString(cbbFlag))
               .AddCgiParam("range_txt",   text)
               .AddCgiParam("operation",   "del");

            cbbRes = DoCbbRequest(req, options);
        } catch (...) {
            EVLOG_MSG << "CBB API network error on remove block for "
                        << ", flag: " << cbbFlag
                        << ", text: " << text
                        << ", exception: " << CurrentExceptionMessage();
            return false;
        }

        if (StripString(cbbRes) != OK) {
            throw TCbbApiError() << "CBB API error: could not remove block for "
                                << "'" << text << "'"
                                << ", flag: " << cbbFlag
                                << ", server answer: " << cbbRes;
        }
        return true;

    }

    bool AddCbbBlock(TCbbGroupId cbbFlag, const TAddr& srcAddr, const TAddr& dstAddr, TInstant expireTime, const TString& description, const TCbbIO::TOptions& options, const TString& tvmTicket) {
        TString cbbRes;

        Y_ENSURE(srcAddr.GetFamily() == dstAddr.GetFamily());

        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "set_range.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag", ToString(cbbFlag))
               .AddCgiParam("range_src", srcAddr.ToString())
               .AddCgiParam("range_dst", dstAddr.ToString())
               .AddCgiParam("operation", "add")
               .AddCgiParam("expire", ToString(expireTime.TimeT()))
               .AddCgiParam("description", description);
            if (srcAddr.GetFamily() == AF_INET6) {
                req.AddCgiParam("version", "6");
            }

            cbbRes =  DoCbbRequest(req, options);


        } catch (...) {
            EVLOG_MSG << "CBB API network error on add block for range"
                        << " from " << srcAddr.ToString()
                        << " to " << dstAddr.ToString()
                        << ", flag: " << cbbFlag
                        << ", exception: " << CurrentExceptionMessage();
            return false;
        }

        if (StripString(cbbRes) != OK) {
            throw TCbbApiError()
                << "CBB API error: could not add range"
                << " from " << srcAddr.ToString()
                << " to " << dstAddr.ToString()
                << ", flag: " << cbbFlag
                << ", server answer: " << cbbRes;
        }

        return true;
    }

    bool AddCbbIps(TCbbGroupId cbbFlag, const TVector<TAddr>& addrs, const TCbbIO::TOptions& options, const TString& tvmTicket) {
        TString cbbRes;

        try {
            TStringStream ss;
            for (const auto& addr : addrs) {
                ss << addr << "\n";
            }

            auto req = HttpPost(options.CbbApiHost, CBB_API_PREFIX + "add_ips.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
                .AddCgiParam("flag", ToString(cbbFlag))
                .AddCgiParam("user", "robot-ne-robot")
                .SetContent(ss.Str());

            cbbRes = DoCbbRequest(req, options);
        } catch (...) {
            EVLOG_MSG << "CBB API network error on add ips,"
                        << ", size: " << addrs.size()
                        << ", flag: " << cbbFlag
                        << ", exception: " << CurrentExceptionMessage();
            return false;
        }

        if (StripString(cbbRes) != OK) {
            throw TCbbApiError()
                << "CBB API error: could not add ips"
                << " ip size; " << addrs.size()
                << ", flag: " << cbbFlag
                << ", server answer: " << cbbRes;
        }

        return true;

    }

    bool AddCbbTxtBlock(TCbbGroupId cbbFlag, const TString& txtBlock, const TString& description, const TCbbIO::TOptions& options, const TString& tvmTicket) {
        TString cbbRes;

        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "set_range.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag", ToString(cbbFlag))
               .AddCgiParam("range_txt", txtBlock)
               .AddCgiParam("operation", "add")
               .AddCgiParam("description", description);

            cbbRes =  DoCbbRequest(req, options);
        } catch (...) {
            EVLOG_MSG << "CBB API network error on add block for {" << txtBlock << "}"
                        << ", flag: " << cbbFlag
                        << ", exception: " << CurrentExceptionMessage();
            return false;
        }

        if (StripString(cbbRes) != OK) {
            throw TCbbApiError()
                << "CBB API error: could not add '" << txtBlock << "'"
                << ", flag: " << cbbFlag
                << ", server answer: " << cbbRes;
        }

        return true;
    }

    bool ReadCbbExpiredList(TCbbGroupId flag, TAddrSet& addrSet, const TCbbIO::TOptions& options, const TString& tvmTicket) {
        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "get_range.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag", ToString(flag))
               .AddCgiParam("with_format", "range_src,range_dst,expire");

            NParseCbb::ParseAddrList(addrSet, DoCbbRequest(req, options));

            req.AddCgiParam("version", "6");
            NParseCbb::ParseAddrList(addrSet, DoCbbRequest(req, options));

            return true;
        } catch (...) {
            EVLOG_MSG << "CBB API network error on getting block list"
                        << ", flag: " << flag
                        << ", exception: " << CurrentExceptionMessage();
        }
        return false;
    }

    TVector<TInstant> GetLastChangeOfFlag(
        const TVector<TCbbGroupId>& ids,
        const TCbbIO::TOptions& options,
        const TString& tvmTicket
    ) {
        TStringBuilder flag;

        for (auto id : ids) {
            flag << id << ',';
        }

        flag.pop_back();

        try {
            auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "check_flag.pl");
            req.AddHeader(X_YA_SERVICE_TICKET, tvmTicket)
               .AddCgiParam("flag", flag);

            const auto response = DoCbbRequest(req, options);
            TVector<TInstant> timestamps;

            for (const auto& token : StringSplitter(response).Split('\n').SkipEmpty()) {
                timestamps.push_back(TInstant::Seconds(FromString<ui64>(token.Token())));
            }

            return timestamps;
        } catch (...) {
            EVLOG_MSG << "CBB error on getting time of last change"
                        << ", flag: " << flag
                        << ", exception: " << CurrentExceptionMessage();
        }
        return {};
    }

    class TCbbHelper {
        TAtomic CbbErrors;
        TCbbIO::TOptions Options;
    public:
        TCbbHelper(const TCbbIO::TOptions& options)
          : CbbErrors(0)
          , Options(options)
        {
        }

        void IncCbbErrors() {
            AtomicIncrement(CbbErrors);
        }

        const TCbbIO::TOptions& GetOptions() const {
            return Options;
        }

        ui64 GetCbbErrors() const {
            return AtomicGet(CbbErrors);
        }
    };

    class TCbbIO::TImpl {
        static const size_t MaxQueueSize = 500;
        TAtomicSharedPtr<TCbbHelper> Helper;
        TThreadPool CmdQueue;
        const TAntirobotTvm* Tvm;
    private:
        TString GetServiceTicket() {
            return Tvm->GetServiceTicket(ANTIROBOT_DAEMON_CONFIG.CbbTVMClientId);
        }
    public:
        TImpl(const TCbbIO::TOptions& options, const TAntirobotTvm* tvm)
          : Helper(new TCbbHelper(options))
          , Tvm(tvm)
        {
            CmdQueue.Start(1, MaxQueueSize);
        }

        ~TImpl() {
            CmdQueue.Stop();
        }

        TFuture<void> AddBlock(TCbbGroupId cbbFlag, const TAddr& srcAddr, const TAddr& dstAddr, TInstant expireTime, const TString& description) {
            TInstant now = TInstant::Now();

            return SafeAsync([=] {
                if (now >= expireTime) {
                    EVLOG_MSG << "CBB: skipped add block for range"
                              << " from " << srcAddr.ToString()
                              << " to " << dstAddr.ToString()
                              << " because ExpireTime is expired "
                              << "(now: " << now << ", ExpireTime: " << expireTime
                              << ", task creation time: " << now << ")";
                    return;
                }

                if (!AddCbbBlock(cbbFlag, srcAddr, dstAddr, expireTime, description, Helper->GetOptions(), GetServiceTicket())) {
                    Helper->IncCbbErrors();
                }
            }, CmdQueue);
        }

        TFuture<void> AddIps(TCbbGroupId cbbFlag, const TVector<TAddr>& addrs) {
            return SafeAsync([=] {
                if (!AddCbbIps(cbbFlag, addrs, Helper->GetOptions(), GetServiceTicket())) {
                    Helper->IncCbbErrors();
                }
            }, CmdQueue);
        }

        TFuture<void> AddTxtBlock(TCbbGroupId cbbFlag, const TString& text, const TString& description) {
            return SafeAsync([=] {
                if (!AddCbbTxtBlock(cbbFlag, text, description, Helper->GetOptions(), GetServiceTicket())) {
                    Helper->IncCbbErrors();
                }
            }, CmdQueue);
        }

        TFuture<TVector<TInstant>> CheckFlags(TVector<TCbbGroupId> ids) {
            return SafeAsync([this, ids = std::move(ids)] {
                const auto& options = Helper->GetOptions();
                const auto ticket = GetServiceTicket();
                return GetLastChangeOfFlag(ids, options, ticket);
            }, CmdQueue);
        }

        TFuture<TString> ReadList(
            TCbbGroupId cbbFlag,
            const TString& format
        ) {
            return SafeAsync([=] {
                const auto& options = Helper->GetOptions();
                const auto ticket = GetServiceTicket();

                auto req = HttpGet(options.CbbApiHost, CBB_API_PREFIX + "get_range.pl");
                req.AddHeader(X_YA_SERVICE_TICKET, ticket)
                    .AddCgiParam("flag", ToString(cbbFlag))
                    .AddCgiParam("with_format", format)
                    .AddCgiParam("version", "all");

                try {
                    return DoCbbRequest(req, options);
                } catch (...) {
                    Helper->IncCbbErrors();
                    throw;
                }
            }, CmdQueue);
        }

        TMaybe<TCbbAddrSet> ReadExpiredListImpl(TCbbGroupId cbbFlag, TInstant timeStamp, const TString& tvmTicket, TCbbHelper* helper) {
            TVector<TInstant> newTimestamps = GetLastChangeOfFlag({cbbFlag}, helper->GetOptions(), tvmTicket);
            const auto newTimestamp = newTimestamps[0];

            if (newTimestamp == TInstant::Zero() || timeStamp < newTimestamp) {
                TAddrSet cbbFlagContent;
                if (ReadCbbExpiredList(cbbFlag, cbbFlagContent, helper->GetOptions(), tvmTicket)) {
                    return TCbbAddrSet(std::move(cbbFlagContent), {cbbFlag});
                } else {
                    helper->IncCbbErrors();
                }
            }
            return Nothing();
        }

        TFuture<TMaybe<TCbbAddrSet>> ReadExpiredList(TCbbGroupId cbbFlag, TInstant timeStamp) {
            return SafeAsync([=] {
                return ReadExpiredListImpl(cbbFlag, timeStamp, GetServiceTicket(), Helper.Get());
            }, CmdQueue);
        }

        TFuture<TMaybe<TVector<TString>>> ReadReList(TCbbGroupId cbbFlag) {
            static const TString format = "range_re";

            return SafeAsync([=] () -> TMaybe<TVector<TString>> {
                TVector<TString> list;

                if (!ReadCbbStringList(
                    cbbFlag, list,
                    Helper->GetOptions(),
                    GetServiceTicket(),
                    format, "re list"
                )) {
                    Helper->IncCbbErrors();
                    return Nothing();
                }

                return list;
            }, CmdQueue);
        }

        TFuture<TMaybe<TVector<TString>>> ReadTextList(TCbbGroupId cbbFlag) {
            static const TString format = "range_txt,rule_id";

            return SafeAsync([=] () -> TMaybe<TVector<TString>> {
                TVector<TString> list;

                if (!ReadCbbStringList(
                    cbbFlag, list,
                    Helper->GetOptions(),
                    GetServiceTicket(),
                    format, "text list"
                )) {
                    Helper->IncCbbErrors();
                    return Nothing();
                }

                return list;
            }, CmdQueue);
        }

        TFuture<void> RemoveTxtBlock(TCbbGroupId cbbFlag, const TString& txtBlock) {
            return SafeAsync([=] {
                if (!RemoveCbbTxtBlock(cbbFlag, txtBlock, Helper->GetOptions(), GetServiceTicket())) {
                    Helper->IncCbbErrors();
                }
            }, CmdQueue);
        }

        void PrintStats(TStatsWriter& out) const {
            out.WriteScalar("cbb_errors",  Helper->GetCbbErrors())
               .WriteHistogram("cbb_queue_size",  CmdQueue.Size());
        }

        size_t QueueSize() const {
            return CmdQueue.Size();
        }
    };

    TCbbIO::TCbbIO(const TOptions& options, const TAntirobotTvm* tvm)
        : Impl(new TImpl(options, tvm))
    {
    }

    TCbbIO::~TCbbIO() = default;

    TFuture<void> TCbbIO::AddBlock(TCbbGroupId cbbFlag, const TAddr& addr, TInstant expireTime, const TString& description) {
        return Impl->AddBlock(cbbFlag, addr, addr, expireTime, description);
    }

    TFuture<void> TCbbIO::AddIps(TCbbGroupId cbbFlag, const TVector<TAddr>& addrs) {
        return Impl->AddIps(cbbFlag, addrs);
    }

    TFuture<void> TCbbIO::AddRangeBlock(TCbbGroupId cbbFlag, const TAddr& srcAddr, const TAddr& dstAddr, TInstant expireTime, const TString& description) {
        return Impl->AddBlock(cbbFlag, srcAddr, dstAddr, expireTime, description);
    }

    TFuture<void> TCbbIO::AddTxtBlock(TCbbGroupId cbbFlag, const TString& text, const TString& description) {
        return Impl->AddTxtBlock(cbbFlag, text, description);
    }

    TFuture<TVector<TInstant>> TCbbIO::CheckFlags(TVector<TCbbGroupId> ids) {
        return Impl->CheckFlags(std::move(ids));
    }

    TFuture<TString> TCbbIO::ReadList(
        TCbbGroupId cbbFlag,
        const TString& format
    ) {
        return Impl->ReadList(cbbFlag, format);
    }

    TFuture<TMaybe<TCbbAddrSet>> TCbbIO::ReadExpiredList(TCbbGroupId cbbFlag, TInstant timeStamp) {
        return Impl->ReadExpiredList(cbbFlag, timeStamp);
    }

    TFuture<TMaybe<TVector<TString>>> TCbbIO::ReadReList(TCbbGroupId flag) {
        return Impl->ReadReList(flag);
    }

    TFuture<TMaybe<TVector<TString>>> TCbbIO::ReadTextList(TCbbGroupId cbbFlag) {
        return Impl->ReadTextList(cbbFlag);
    }

    TFuture<void> TCbbIO::RemoveTxtBlock(TCbbGroupId cbbFlag, const TString& txtBlock) {
        return Impl->RemoveTxtBlock(cbbFlag, txtBlock);
    }

    void TCbbIO::PrintStats(TStatsWriter& out) const {
        Impl->PrintStats(out);
    }

    size_t TCbbIO::QueueSize() const {
        return Impl->QueueSize();
    }

    TCbbCacheIO::TCbbCacheIO(TCbbCache* cache)
        : Cache(cache) {
    }

    TFuture<TString> TCbbCacheIO::ReadList(
        TCbbGroupId cbbFlag,
        const TString& format
    ) {
        THttpRequest req("GET", "localhost", "/cgi-bin/get_range.pl");
        req.AddCgiParam("flag", ToString(cbbFlag))
            .AddCgiParam("with_format", format)
            .AddCgiParam("version", "all");

        const auto res = Cache->Get(req.GetPathAndQuery());
        if (!res.Defined()) {
            return {};
        }

        TString result = *res.Get();

        return MakeFuture<TString>(std::move(result));
    }

    TFuture<TVector<TInstant>> TCbbCacheIO::CheckFlags(TVector<TCbbGroupId> ids) {
        TVector<TInstant> timestamps;
        timestamps.reserve(ids.size());

        TStringBuilder flag;
        for (auto id : ids) {
            flag << id << ',';
        }
        flag.pop_back();

        THttpRequest req("GET", "localhost", "/cgi-bin/check_flag.pl");
        req.AddCgiParam("flag", ToString(flag));

        if (const auto res = Cache->Get(req.GetPathAndQuery()); res.Defined()) {
            for (const auto& token : StringSplitter(*res.Get()).Split('\n').SkipEmpty()) {
                timestamps.push_back(TInstant::Seconds(FromString<ui64>(token.Token())));
            }
        }

        return MakeFuture<TVector<TInstant>>(std::move(timestamps));
    }

} // namespace NAntiRobot
