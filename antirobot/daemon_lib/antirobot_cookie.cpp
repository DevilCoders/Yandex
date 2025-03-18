#include "antirobot_cookie.h"

#include <antirobot/idl/antirobot_cookie.pb.h>

#include <antirobot/lib/evp.h>
#include <antirobot/lib/enum.h>

#include <openssl/rand.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/system/byteorder.h>
#include <util/system/unaligned_mem.h>

#include <array>
#include <type_traits>


namespace NAntiRobot {


TLastVisitsCookie::TLastVisitsCookie(
    const THashSet<TRuleId>& ids,
    TStringBuf bytes
) {
    Y_ENSURE(
        bytes.size() >= sizeof(ui32),
        "Invalid last visits subcookie size"
    );

    LastUpdateSeconds = LittleToHost(ReadUnaligned<ui32>(bytes.data()));

    NProtoBuf::io::CodedInputStream entryInput(
        reinterpret_cast<const NProtoBuf::uint8*>(bytes.data() + sizeof(ui32)),
        static_cast<int>(bytes.size() - sizeof(ui32))
    );

    std::underlying_type_t<TRuleId> idRep;
    ui8 lastVisit;
    while (entryInput.ReadVarint32(&idRep) && entryInput.ReadRaw(&lastVisit, 1)) {
        const TRuleId id{idRep};
        if (ids.contains(id)) {
            IdToLastVisit[id] = lastVisit;
        }
    }
}

bool TLastVisitsCookie::IsStale(TInstant now) const {
    static constexpr ui32 LAST_VISITS_AGE_THRESHOLD_SECONDS = 300;

    if (LastUpdateSeconds == 0) {
        return true;
    }

    const auto lastUpdate = ANTIROBOT_COOKIE_EPOCH_START + TDuration::Seconds(LastUpdateSeconds);
    return now > lastUpdate && (now - lastUpdate).Seconds() > LAST_VISITS_AGE_THRESHOLD_SECONDS;
}

bool TLastVisitsCookie::Touch(TConstArrayRef<TRuleId> ids, TInstant now) {
    bool dirty = false;

    const ui32 secondsNow = (now - ANTIROBOT_COOKIE_EPOCH_START).Seconds();
    const ui32 hoursDiff = secondsNow / 3600 - LastUpdateSeconds / 3600;

    LastUpdateSeconds = secondsNow;

    if (hoursDiff > 255) {
        IdToLastVisit.clear();
        dirty = true;
    } else {
        for (auto& [id, lastVisit] : IdToLastVisit) {
            if (lastVisit != 0) {
                const ui32 unclampedNewLastVisit = lastVisit + hoursDiff;
                const ui32 newLastVisit = unclampedNewLastVisit > 255 ? 255 : unclampedNewLastVisit;
                dirty = dirty || (lastVisit != newLastVisit);
                lastVisit = newLastVisit;
            }
        }
    }

    for (const auto id : ids) {
        auto& lastVisit = IdToLastVisit[id];
        dirty = dirty || (lastVisit != 1);
        lastVisit = 1;
    }

    return dirty;
}

void TLastVisitsCookie::Serialize(NProtoBuf::io::ZeroCopyOutputStream* output) const {
    NProtoBuf::io::CodedOutputStream codedOutput(output);

    const auto littleTimestamp = HostToLittle(LastUpdateSeconds);
    codedOutput.WriteRaw(&littleTimestamp, sizeof(littleTimestamp));

    for (const auto& [id, lastVisit] : IdToLastVisit) {
        codedOutput.WriteVarint32(EnumValue(id));
        codedOutput.WriteRaw(&lastVisit, 1);

        if (codedOutput.HadError()) {
            break;
        }
    }

    Y_ENSURE(!codedOutput.HadError(), "Failed to serialize last visits cookie");
}


TAntirobotCookie::TAntirobotCookie(
    const THashSet<TLastVisitsCookie::TRuleId>& lastVisitsRuleIds,
    TStringBuf bytes
) {
    NAntirobotCookieProto::TAntirobotCookie pbAntirobotCookie;

    Y_ENSURE(
        pbAntirobotCookie.ParseFromArray(bytes.data(), bytes.size()),
        "Invalid antirobot cookie protobuf"
    );

    LastVisitsCookie = TLastVisitsCookie(lastVisitsRuleIds, pbAntirobotCookie.GetLastVisits());
}

TString TAntirobotCookie::Serialize() const {
    NAntirobotCookieProto::TAntirobotCookie pbAntirobotCookie;
    NProtoBuf::io::StringOutputStream lastVisitsOutput(pbAntirobotCookie.MutableLastVisits());
    LastVisitsCookie.Serialize(&lastVisitsOutput);

    return pbAntirobotCookie.SerializeAsString();
}

TString TAntirobotCookie::Encrypt(TStringBuf key) const {
    std::array<ui8, EVP_RECOMMENDED_GCM_IV_LEN> iv;
    Y_ENSURE(
        RAND_bytes(iv.data(), iv.size()) == 1,
        "RAND_bytes failed"
    );

    const TStringBuf ivStr(reinterpret_cast<char*>(iv.data()), iv.size());
    TEvpEncryptor encryptor(EEvpAlgorithm::Aes256Gcm, key, ivStr);
    const TString encrypted = encryptor.Encrypt(Serialize());

    return Base64Encode(ivStr + encrypted);
}

TAntirobotCookie TAntirobotCookie::Decrypt(
    TStringBuf key,
    const THashSet<TLastVisitsCookie::TRuleId>& ids,
    TStringBuf cookie
) {
    const auto decodedCookie = Base64StrictDecode(cookie);

    Y_ENSURE(
        decodedCookie.size() >= EVP_RECOMMENDED_GCM_IV_LEN,
        "antirobot cookie too short"
    );

    TStringBuf iv, data;
    TStringBuf(decodedCookie).SplitAt(EVP_RECOMMENDED_GCM_IV_LEN, iv, data);

    TEvpDecryptor decryptor(EEvpAlgorithm::Aes256Gcm, key, iv);
    const auto decryptedCookie = decryptor.Decrypt(data);

    return TAntirobotCookie(ids, decryptedCookie);
}

void AddCookieSuffix(IOutputStream& output, const TStringBuf& domain, const TInstant& expires) {
    if (!domain.empty()) {
        output << "; domain=." << domain;
    }

    struct tm tM;
    output << "; path=/; expires=" << Strftime("%a, %d-%b-%Y %H:%M:%S GMT", expires.GmTime(&tM));
}

} // namespace NAntiRobot


template <>
void Out<NAntiRobot::TLastVisitsCookie::TRuleId>(
    IOutputStream& out,
    NAntiRobot::TLastVisitsCookie::TRuleId id
) {
    out << EnumValue(id);
}
