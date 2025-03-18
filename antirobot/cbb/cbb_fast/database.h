#pragma once

#include <antirobot/cbb/cbb_fast/protos/config.pb.h>

#include <antirobot/lib/addr.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/system/tempfile.h>

#include <utility>


namespace maps::pgpool3 {
    class Pool;
} // namespace maps::pgpool3


namespace pqxx {
    class row;
} // namespace pgxx


namespace NCbbFast {

struct TAddIpsParams {
    ui32 Flag = 0;
    ui32 MaxIps = 2000;
    TDuration Timeout = TDuration::Minutes(10);
    TString User;
    TVector<TString> Addrs;
};

class TBaseRule {
protected:
    TBaseRule() = default;
    explicit TBaseRule(const pqxx::row& row, bool hasId = true);

public:
    ui32 Id = 0;
    TInstant Created;
    TInstant Expired;
};


class TLineRule : public TBaseRule {
public:
    TLineRule() = default;
    explicit TLineRule(const pqxx::row& row);

public:
    TString RangeLine;
};


class TIpRule : public TBaseRule {
public:
    TIpRule() = default;

    static TIpRule FromIpv4(const pqxx::row& row);
    static TIpRule FromIpv6(const pqxx::row& row);

private:
    TIpRule(const pqxx::row& row, NAntiRobot::TAddr start, NAntiRobot::TAddr end);

public:
    NAntiRobot::TAddr RangeStart;
    NAntiRobot::TAddr RangeEnd;
    TString Serialized;
};


class TGroup {
public:
    enum EType {
        Re,
        Txt,
        Ip
    };

    using TRules = std::variant<
        std::monostate,
        TVector<TLineRule>,
        TVector<TIpRule>
    >;

public:
    TGroup() = default;

    explicit TGroup(
        EType type,
        TInstant updated,
        TRules rules
    )
        : Type(type)
        , Updated(updated)
        , Checked(TInstant::Now())
        , Rules(std::move(rules))
    {}

public:
    EType Type;
    TInstant Updated;
    TInstant Checked;
    TRules Rules;
};

using TUpdatedTimes = TMap<ui32, TInstant>;

class TDatabase {
public:
    explicit TDatabase(const TConfig& config);

    TMaybe<TGroup> CheckFlag(ui32 groupId);
    TUpdatedTimes GetUpdated();

    bool GetRange(ui32 groupId, TGroup* group);
    bool AddIps(const TAddIpsParams& params);

private:
    TGroup::TRules GetRangeLine(TStringBuf query, ui32 groupId);
    TGroup::TRules GetRangeIp(ui32 groupId);

private:
    TTempFileHandle CertFile;
    TAtomicSharedPtr<maps::pgpool3::Pool> Pool;
};


} // namespace NCbbFast
