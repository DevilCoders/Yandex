#include "normalizer.h"

#include <util/charset/unidata.h>
#include <util/string/cast.h>

static TString IdToLower(TString id) {
    TString result(id);
    result.to_lower();
    return result;
}

static TString UnderScoreToHyphen(TString id) {
    TString result;
    for (size_t i = 0; i < id.size(); ++i) {
        if (id[i] == '_') {
            result.append('-');
        } else {
            result.append(id[i]);
        }
    }

    return IdToLower(result);
}

static bool IsAllDigit(TString s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (!IsDigit(s[i])) {
            return false;
        }
    }

    return true;
}

static TString MoiKrug(TString id) {
    if (id[0] == 'P' && IsAllDigit(id.substr(1))) {
        return id;
    }
    return IdToLower(id);
}

static TString VKontakte(TString id) {
    TString lowercased = IdToLower(id);
    if (IsAllDigit(lowercased)) {
        lowercased = "id" + lowercased;
    }
    if (lowercased.StartsWith("id0")) {
        int idNum;
        try {
            idNum = FromString(lowercased.substr(2));
        } catch (...) {
            return TString();
        }
        lowercased = "id" + ToString(idNum);
    }

    if (lowercased != "id") {
        return lowercased;
    } else {
        return TString();
    }
}

static TString Odnoklassniki(TString id) {
    const TString& lowercased = IdToLower(id);
    if (IsAllDigit(lowercased)) {
        return "profile/" + lowercased;
    }
    if (lowercased.StartsWith("user/")) {
        if (IsAllDigit(lowercased.substr(5))) {
            return "profile/" + lowercased.substr(5);
        } else {
            return lowercased.substr(5);
        }
    }
    return lowercased;
}

namespace NSocial {

TIdentityNormalizer::TIdentityNormalizer() {
    auto trivial = [] (const TString& id) {return id;};

    Normalizers[NSocial::Beon] =  UnderScoreToHyphen;
    Normalizers[NSocial::DiaryRu] = IdToLower;
    Normalizers[NSocial::Facebook] = IdToLower;
    Normalizers[NSocial::FreeLance] = IdToLower;
    Normalizers[NSocial::FriendFeed] = IdToLower;
    Normalizers[NSocial::GooglePlus] = trivial;
    Normalizers[NSocial::LinkedIn] = IdToLower;
    Normalizers[NSocial::LiveInternet] = IdToLower;
    Normalizers[NSocial::LiveJournal] = UnderScoreToHyphen;
    Normalizers[NSocial::MirTesen] = trivial;
    Normalizers[NSocial::MoiKrug] = ::MoiKrug;
    Normalizers[NSocial::MoiMirMailRu] = IdToLower;
    Normalizers[NSocial::Twitter] = IdToLower;
    Normalizers[NSocial::VKontakte] = ::VKontakte;
    Normalizers[NSocial::YaRu] = UnderScoreToHyphen;
    Normalizers[NSocial::Odnoklassniki] = ::Odnoklassniki;
    Normalizers[NSocial::Foursquare] = IdToLower;
    Normalizers[NSocial::Instagram] = IdToLower;
}

TIdentityNormalizer*TIdentityNormalizer::Instance() {
    return Singleton<TIdentityNormalizer>();
}

TString TIdentityNormalizer::Normalize(ESocialNetwork network, const TString& id) {
    if (network < NSocial::NETWORKS_COUNT) {
        return Normalizers[network](id);
    } else {
        ythrow yexception() << "Unknown ESocialNetwork const";
    }
}

} // namespace NSocial
