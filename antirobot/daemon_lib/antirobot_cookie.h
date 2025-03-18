#pragma once

#include <google/protobuf/io/coded_stream.h>

#include <util/datetime/base.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash_set.h>
#include <util/generic/strbuf.h>


namespace NAntiRobot {


constexpr TInstant ANTIROBOT_COOKIE_EPOCH_START = TInstant::Seconds(1621900800);

constexpr TStringBuf ANTIROBOT_COOKIE_KEY = "_yasc";


class TLastVisitsCookie {
public:
    enum class TRuleId : ui32 {};

public:
    TLastVisitsCookie() = default;

    TLastVisitsCookie(ui32 lastUpdateSeconds, THashMap<TRuleId, ui8> idToLastVisit)
        : LastUpdateSeconds(lastUpdateSeconds)
        , IdToLastVisit(std::move(idToLastVisit))
    {}

    explicit TLastVisitsCookie(
        const THashSet<TRuleId>& ids,
        TStringBuf bytes
    );

    ui32 GetLastUpdateSeconds() const {
        return LastUpdateSeconds;
    }

    const THashMap<TRuleId, ui8>& GetIdToLastVisit() const {
        return IdToLastVisit;
    }

    ui8 Get(TRuleId id) const {
        if (const auto lastVisit = IdToLastVisit.FindPtr(id)) {
            return *lastVisit;
        }

        return 0;
    }

    bool IsStale(TInstant now = TInstant::Now()) const;

    bool Touch(TConstArrayRef<TRuleId> ids, TInstant now = TInstant::Now());

    void Serialize(NProtoBuf::io::ZeroCopyOutputStream* output) const;

private:
    ui32 LastUpdateSeconds = 0;
    THashMap<TRuleId, ui8> IdToLastVisit;
};


struct TAntirobotCookie {
    TAntirobotCookie() = default;

    explicit TAntirobotCookie(TLastVisitsCookie lastVisitsCookie)
        : LastVisitsCookie(std::move(lastVisitsCookie))
    {}

    explicit TAntirobotCookie(
        const THashSet<TLastVisitsCookie::TRuleId>& lastVisitsRuleIds,
        TStringBuf bytes
    );

    TString Serialize() const;

    TString Encrypt(TStringBuf key) const;

    static TAntirobotCookie Decrypt(
        TStringBuf key,
        const THashSet<TLastVisitsCookie::TRuleId>& ids,
        TStringBuf cookie
    );

    TLastVisitsCookie LastVisitsCookie;
};

void AddCookieSuffix(IOutputStream& output, const TStringBuf& domain, const TInstant& expires);

} // namespace NAntiRobot
