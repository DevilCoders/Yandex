#include "server.h"

#include "antirobot/lib/addr.h"
#include "env.h"
#include "library/cpp/http/misc/httpcodes.h"
#include "util/generic/yexception.h"

#include <antirobot/cbb/cbb_fast/protos/cbb_response.pb.h>

#include <google/protobuf/messagext.h>

#include <kernel/server/server.h>
#include <kernel/server/protos/serverconf.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/iterator/mapped.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/spinlock.h>


namespace NCbbFast {

namespace {

constexpr TDuration TimeOffset = TDuration::Hours(3);

template <typename TContainer, typename TDelim, typename F>
void MapJoin(const TContainer& xs, TDelim delim, TStringBuilder* s, const F& f) {
    if (xs.empty()) {
        return;
    }

    const auto end = std::prev(xs.end());

    for (auto it = xs.begin(); it != end; ++it) {
        if (f(*it, s)) {
            (*s) << delim;
        }
    }

    f(*end, s);
}

template <typename TContainer, typename TDelim, typename F>
TString MapJoin(const TContainer& xs, TDelim delim, const F& f) {
    TStringBuilder s;
    MapJoin(xs, delim, &s, f);

    return std::move(s);
}


class TCbbRequest : public NServer::TRequest {
public:
    explicit TCbbRequest(NServer::TServer& server, TEnv* env)
        : NServer::TRequest{server}
        , Env(env)
    {}

    bool DoReply(const TString& script, THttpResponse& response) override {
        try {
            if (script == "/cgi-bin/check_flag.pl") {
                response = CheckFlag();
            } else if (script == "/cgi-bin/get_range.pl") {
                response = GetRange();
            } else if (script == "/cgi-bin/get_ips.pl") {
                response = GetIps();
            } else if (script == "/cgi-bin/add_ips.pl") {
                response = AddIps();
            } else {
                response = TextResponse("Not found\n", HTTP_NOT_FOUND);
            }
        } catch (...) {
            response = TextResponse(TString(CurrentExceptionMessage()) + "\n", HTTP_BAD_REQUEST);
        }

        return true;
    }

private:
    struct TCheckFlagParams {
        TVector<ui32> Flags;
    };

    struct TGetRangeParams {
        enum class EVersion {
            Ipv4,
            Ipv6,
            All
        };

        enum class EFormat {
            RuleId,
            Expire,
            RangeSrc,
            RangeDst,
            RangeRe,
            RangeTxt
        };

        ui32 Flag = 0;
        TVector<EFormat> WithFormat;
        bool NeedRuleId = false;
        TGroup::EType Type;
        EVersion Version = EVersion::Ipv4;
    };

    struct TGetIpsParams {
        ui32 Flag = 0;
        TInstant CreatedAfter = TInstant::Zero();
        bool JsonOutput = false;
    };

private:
    THttpResponse CheckFlag() const {
        const auto params = ParseCheckFlagParams();

        if (
            const TInstant updatedTs = Env->UpdatedTs;
            updatedTs == TInstant::Zero() ||
            updatedTs + CheckPeriod < TInstant::Now()
        ) {
            if (TTryGuard<TAdaptiveLock> guard{Env->UpdatedLock}) {
                Env->Updated.Set(Env->Database.GetUpdated());
                Env->UpdatedTs = TInstant::Now();
            }
        }

        return TextResponse(MapJoin(
            params.Flags, '\n',
            [this] (const auto& flag, const auto builder) -> bool {
                const auto* it = Env->Updated.Get()->FindPtr(flag);
                if (it) {
                    (*builder) << it->Seconds();
                } else {
                    (*builder) << 0;
                }
                return true;
            }
        ));
    }

    TCheckFlagParams ParseCheckFlagParams() const {
        TCheckFlagParams params;
        const auto& cgiParams = RequestData().CgiParam;

        Y_ENSURE(
            cgiParams.size() == 1 && cgiParams.begin()->first == "flag",
            "Expected a single parameter 'flag'"
        );

        const auto& flags = cgiParams.begin()->second;

        for (const auto& state : StringSplitter(flags).Split(',')) {
            ui32 flag;
            Y_ENSURE(TryFromString(state.Token(), flag), "Invalid parameter 'flag'");

            params.Flags.push_back(flag);
        }

        return params;
    }

    THttpResponse GetRange() const {
        const auto params = ParseGetRangeParams();
        const auto group = GetGroup(params.Flag);

        Y_ENSURE(params.Type == group->Type, "Invalid format combination for this group type");

        return TextResponse(std::visit([&params] <typename T> (const T& rules) -> TString {
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            } else {
                return GetRange(rules, params);
            }
        }, group->Rules));
    }

    THttpResponse GetIps() const {
        const auto params = ParseGetIpsParams();
        const auto group = GetGroup(params.Flag);

        Y_ENSURE(group->Type == TGroup::EType::Ip, "Invalid format for this group type");

        return TextResponse(std::visit([&params] <typename T> (const T& rules) -> TString {
            if constexpr (std::is_same_v<T, TVector<TIpRule>>) {
                return GetIps(rules, params);
            } else {
                return {};
            }
        }, group->Rules));
    }

    THttpResponse AddIps() const {
        try {
            const auto ticket = RequestData().HeaderInOrEmpty(X_YA_SERVICE_TICKET);
            if (ticket.empty()) {
                return TextResponse("Authentication failed. Invalid secret.\n", HTTP_FORBIDDEN);
            }

            Env->Tvm.CheckClientTicket(ticket);

            const auto& params = ParseAddIpsParams();

            const auto group = GetGroup(params.Flag);
            Y_ENSURE(group->Type == TGroup::EType::Ip, "Invalid format for this group type");


            Env->Database.AddIps(params);

            return TextResponse("Ok\n", HTTP_OK);
        } catch (const TInvalidTvmClientException&) {
            return TextResponse("Authentication failed. Invalid secret.\n", HTTP_FORBIDDEN);
        }
    }

    static TString GetIps(const TVector<TIpRule>& rules, const TGetIpsParams& params) {
        TStringStream output;
        NProtoBuf::io::TCopyingOutputStreamAdaptor adaptor(&output);
        NProtobufJson::TProto2JsonConfig cfg;

        // don't export unlimited ban
        // skip created before ts
        auto filter = [&params](const TIpRule& rule) {
            return rule.Expired && (params.CreatedAfter == TInstant::Zero() || rule.Created - TimeOffset > params.CreatedAfter);
        };

        {
            ui32 numRules = 0;
            TInstant LastCreated = TInstant::Zero();
            
            for (const auto& rule : rules) {
                if (filter(rule)) {
                    ++numRules;
                    LastCreated = std::max(rule.Created - TimeOffset, LastCreated);
                }
            }

            TCbbIpResponceHeader header;
            header.SetLastCreatedTs(LastCreated.MicroSeconds());
            header.SetNumIps(numRules);

            if (params.JsonOutput) {
                NProtobufJson::Proto2Json(header, output, cfg);
            } else {
                Y_ENSURE(
                    NProtoBuf::io::SerializeToZeroCopyStreamSeq(&header, &adaptor),
                    "Failed to serialize header"
                );
            }
        }

        for (const auto& rule : rules) {
            if (!filter(rule)) {
                continue;
            }

            TCbbIpResponceItem item;
            item.SetIp(rule.Serialized);

            item.SetExpiredTs((rule.Expired - TimeOffset).Seconds());

            if (params.JsonOutput) {
                NProtobufJson::Proto2Json(item, output, cfg);
            } else {
                Y_ENSURE(
                    NProtoBuf::io::SerializeToZeroCopyStreamSeq(&item, &adaptor),
                    "Failed to serialize item"
                );
            }
        }

        if (!params.JsonOutput) {
            adaptor.Flush();
        }

        return output.Str();
    }

    TAddIpsParams ParseAddIpsParams() const {
        TAddIpsParams params;

        const auto& cgiParams = RequestData().CgiParam;

        Y_ENSURE(cgiParams.Has("flag"));
        Y_ENSURE(
            TryFromString(cgiParams.Get("flag"), params.Flag),
            "Invalid parameter 'flag'"
        );

        if (cgiParams.Has("max")) {
            Y_ENSURE(
                TryFromString(cgiParams.Get("max"), params.MaxIps),
                "Invalid parameter 'max'"
            );
        }

        Y_ENSURE(cgiParams.Has("user"));
        params.User = cgiParams.Get("user");

        if (cgiParams.Has("timeout")) {
            ui32 timeoutSeconds = 0;
            Y_ENSURE(
                TryFromString(cgiParams.Get("timeout"), timeoutSeconds),
                "Invalid parameter 'timeout'"
            );
            params.Timeout = TDuration::Seconds(timeoutSeconds);
        }

        TContainerConsumer<TVector<TString>> c(&params.Addrs);
        TSkipEmptyTokens<TContainerConsumer<TVector<TString>> > cc(&c);
        SplitString(Buf.AsCharPtr(), Buf.AsCharPtr() + Buf.Size(), TCharDelimiter<const char>('\n'), cc);

        return params;
    }

    TGetIpsParams ParseGetIpsParams() const {
        static const THashSet<TString> recognizedKeys = {"group_id", "created_after", "hr"};

        const auto& cgiParams = RequestData().CgiParam;

        for (const auto& [key, _] : cgiParams) {
            Y_ENSURE(recognizedKeys.contains(key), "Invalid parameter '" << key << '\'');
        }

        TGetIpsParams params;

        Y_ENSURE(
            TryFromString(cgiParams.Get("group_id"), params.Flag),
            "Invalid parameter 'group_id'"
        );

        if (cgiParams.Has("created_after")) {
            ui64 us;
            Y_ENSURE(
                TryFromString(cgiParams.Get("created_after"), us),
                "Invalid parameter 'created_after'"
            );

            params.CreatedAfter = TInstant::MicroSeconds(us);
        }

        if (cgiParams.Has("hr")) {
            Y_ENSURE(
                TryFromString(cgiParams.Get("hr"), params.JsonOutput),
                "Invalid parameter 'hr'"
            );
        }

        return params;
    }

    TGetRangeParams ParseGetRangeParams() const {
        static const THashSet<TString> recognizedKeys = {"flag", "with_format", "version"};

        const auto& cgiParams = RequestData().CgiParam;

        for (const auto& [key, _] : cgiParams) {
            Y_ENSURE(recognizedKeys.contains(key), "Invalid parameter '" << key << '\'');
        }

        TGetRangeParams params;

        Y_ENSURE(
            TryFromString(cgiParams.Get("flag"), params.Flag),
            "Invalid parameter 'flag'"
        );

        if (cgiParams.Has("version")) {
            params.Version = ParseVersion(cgiParams.Get("version"));
        }

        for (const auto& state : StringSplitter(cgiParams.Get("with_format")).Split(',')) {
            const auto format = ParseFormat(state.Token());

            if (format == TGetRangeParams::EFormat::RuleId) {
                params.NeedRuleId = true;
            } else {
                params.WithFormat.push_back(format);
            }
        }

        params.Type = CheckFormats(params.WithFormat);

        if (params.NeedRuleId) {
            Y_ENSURE(params.Type != TGroup::EType::Ip, "Rule ids are not supported by IP groups");
        }

        return params;
    }

    static TGetRangeParams::EVersion ParseVersion(TStringBuf versionStr) {
        if (versionStr == "all") {
            return TGetRangeParams::EVersion::All;
        }

        ui8 iVersion;
        Y_ENSURE(TryFromString(versionStr, iVersion), "Invalid parameter 'version'");

        switch (iVersion) {
        case 4:
            return TGetRangeParams::EVersion::Ipv4;
        case 6:
            return TGetRangeParams::EVersion::Ipv6;
        default:
            ythrow yexception() << "Invalid parameter 'version'";
        }
    }

    static TGetRangeParams::EFormat ParseFormat(TStringBuf formatStr) {
        static const THashMap<TStringBuf, TGetRangeParams::EFormat> strToFormat = {
            {"rule_id", TGetRangeParams::EFormat::RuleId},
            {"expire", TGetRangeParams::EFormat::Expire},
            {"range_src", TGetRangeParams::EFormat::RangeSrc},
            {"range_dst", TGetRangeParams::EFormat::RangeDst},
            {"range_re", TGetRangeParams::EFormat::RangeRe},
            {"range_txt", TGetRangeParams::EFormat::RangeTxt}
        };

        if (const auto *const format = strToFormat.FindPtr(formatStr)) {
            return *format;
        }

        ythrow yexception() << "Invalid format: " << formatStr;
    }

    static TGroup::EType CheckFormats(const TVector<TGetRangeParams::EFormat>& formats) {
        bool hasRangeRe = false;
        bool hasRangeTxt = false;
        bool hasRangeIp = false;

        for (const auto format : formats) {
            switch (format) {
            case TGetRangeParams::EFormat::RangeRe:
                hasRangeRe = true;
                break;

            case TGetRangeParams::EFormat::RangeTxt:
                hasRangeTxt = true;
                break;

            case TGetRangeParams::EFormat::Expire:
            case TGetRangeParams::EFormat::RangeSrc:
            case TGetRangeParams::EFormat::RangeDst:
                hasRangeIp = true;
                break;

            default:
                break;
            }
        }

        Y_ENSURE(
            hasRangeRe + hasRangeTxt + hasRangeIp == 1,
            "Invalid format combination"
        );

        if (hasRangeRe) {
            return TGroup::EType::Re;
        } else if (hasRangeTxt) {
            return TGroup::EType::Txt;
        } else if (hasRangeIp) {
            return TGroup::EType::Ip;
        } else {
            Y_ENSURE(false); // Not reachable.
        }
    }

    NThreading::TRcuAccessor<TGroup>::TReference GetGroup(ui32 flag) const {
        Y_ENSURE(flag < Env->Groups.size(), "Flag out of bounds");

        auto& sharedGroup = Env->Groups[flag];
        auto group = sharedGroup.Group.Get();

        if (
            group->Checked == TInstant::Zero() ||
            group->Checked + CheckPeriod < TInstant::Now()
        ) {
            if (TTryGuard<TAdaptiveLock> guard{sharedGroup.Lock}) {
                if (
                    auto newGroup = Env->Database.CheckFlag(flag);
                    newGroup && newGroup->Updated > group->Updated
                ) {
                    if (Env->Database.GetRange(flag, &*newGroup)) {
                        sharedGroup.Group.Set(std::move(*newGroup));
                        group = sharedGroup.Group.Get();
                    }
                }
            }
        }

        return group;
    }

    static TString GetRange(
        const TVector<TLineRule>& rules,
        const TGetRangeParams& params
    ) {
        return MapJoin(rules, '\n', [&params] (const auto& rule, const auto builder) -> bool {
            if (params.NeedRuleId) {
                (*builder) << "rule_id=" << rule.Id << "; ";
            }

            (*builder) << rule.RangeLine;
            return true;
        });
    }

    static TString GetRange(
        const TVector<TIpRule>& rules,
        const TGetRangeParams& params
    ) {
        return MapJoin(rules, '\n', [&params] (const auto& rule, const auto builder) -> bool {
            if (rule.RangeStart.IsIp4()) {
                if (params.Version == TGetRangeParams::EVersion::Ipv6) {
                    return false;
                }
            }

            if (rule.RangeStart.IsIp6()) {
                if (params.Version == TGetRangeParams::EVersion::Ipv4) {
                    return false;
                }
            }

            MapJoin(
                params.WithFormat, "; ", builder,
                [&rule] (const auto format, const auto builder) -> bool {
                    switch (format) {
                    case TGetRangeParams::EFormat::Expire:
                        if (rule.Expired == TInstant::Zero()) {
                            (*builder) << 0;
                        } else {
                            (*builder) << (rule.Expired - TimeOffset).Seconds();
                        }
                        break;

                    case TGetRangeParams::EFormat::RangeSrc:
                        (*builder) << rule.RangeStart;
                        break;

                    case TGetRangeParams::EFormat::RangeDst:
                        (*builder) << rule.RangeEnd;
                        break;

                    default:
                        break;
                    }
                    return true;
                }
            );
            return true;
        });
    }

private:
    static constexpr TDuration CheckPeriod = TDuration::Seconds(1);

    TEnv* Env;
};


} // anonymous namespace


TCbbServer::TCbbServer(const NServer::THttpServerConfig& config, TEnv* env)
    : NServer::TServer{config}
    , Env(env) {
}

TClientRequest* TCbbServer::CreateClient() {
    return new TCbbRequest(*this, Env);
}


} // namespace NCbbFast
