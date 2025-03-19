#include "frontend.h"

#include <kernel/common_server/util/types/cast.h>
#include <kernel/common_server/user_role/abstract/abstract.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/logger/global/global.h>
#include <util/string/subst.h>

#include <cmath>

IBaseServer::IBaseServer() {
    Singleton<ICSOperator>()->SetFrontendServer(this);
}

IBaseServer::~IBaseServer() {
    Singleton<ICSOperator>()->SetFrontendServer(nullptr);
}

TCacheWithAge<TString, NJson::TJsonValue>& IBaseServer::GetCache(const TString& cacheId) const {
    {
        TReadGuard rg(Mutex);
        auto it = Caches.find(cacheId);
        if (it != Caches.end()) {
            return it->second;
        }
    }
    TWriteGuard rg(Mutex);
    return Caches[cacheId];
}



void IBaseServer::InitConstants(NFrontend::TConstantsInfoReport& cReport, TAtomicSharedPtr<IUserPermissions> permissions) const {
    Y_UNUSED(cReport);
    Y_UNUSED(permissions);
}

void ICSOperator::SetFrontendServer(const IBaseServer* frontend) {
    CHECK_WITH_LOG((Server == nullptr) != (frontend == nullptr));
    Server = frontend;
}

const IBaseServer& ICSOperator::GetServerImpl() const {
    CHECK_WITH_LOG(!!Server);
    return *Server;
}

template <class TAction>
TString ILocalization::ApplyResourcesImpl(const TString& value, const TString& localizationId, const TAction& stringProcessor) const {
    static const TString resourceMarker = "(resource:";
    size_t idx = value.find(resourceMarker);
    if (idx == TString::npos) {
        return value;
    }
    TStringBuf strValue = value;
    TString text(strValue.Head(idx));
    while (idx != TString::npos) {
        strValue.Skip(idx + resourceMarker.size());
        TStringBuf resName;
        TStringBuf nextValue;
        strValue.TrySplit(')', resName, nextValue);
        TMaybe<TString> resourceStr = GetLocalString(localizationId, TString(resName));
        if (resourceStr) {
            text += stringProcessor(*resourceStr);
        }
        strValue = nextValue;
        idx = strValue.find(resourceMarker);
        text += strValue.Head(idx);
    };
    return text;
}

TString ILocalization::ApplyResources(const TString& value, const TString& localizationId /*= "rus"*/) const {
    const auto action = [](const TString& valueStr) {
        return valueStr;
    };
    return ApplyResourcesImpl(value, localizationId, action);
}

namespace {
    class TScanProcessor: public NJson::IScanCallback {
    private:
        const ILocalization* Localization = nullptr;
        const TString& LocalizationId;

    public:
        bool Do(const TString& /*path*/, NJson::TJsonValue* /*parent*/, NJson::TJsonValue& value) override {
            if (value.IsString()) {
                value = Localization->ApplyResources(value.GetString(), LocalizationId);
            }
            return true;
        }

        TScanProcessor(const ILocalization* localization, const TString& localizationId)
            : Localization(localization)
            , LocalizationId(localizationId)
        {
        }
    };
}

void ILocalization::ApplyResourcesForJson(NJson::TJsonValue& value, const TString& localizationId /*= "rus"*/) const {
    TScanProcessor scanner(this, localizationId);
    value.Scan(scanner);
}

TString ILocalization::ApplyResourcesForJson(const TString& value, const TString& localizationId /*= "rus"*/) const {
    const auto action = [](const TString& valueStr) {
        return NJson::TJsonValue(valueStr).GetStringRobust();
    };
    return ApplyResourcesImpl(value, localizationId, action);
}

TString ILocalization::DaysFormat(const TDuration d, const TString& localizationId /*= "rus"*/) const {
    if (d < TDuration::Days(1)) {
        return HoursFormat(d, localizationId);
    } else {
        const ui32 idxDays = d.Days() % 10;
        const ui32 idxDays100 = d.Hours() % 100;
        if (idxDays == 0 || idxDays >= 5 || (idxDays100 >= 10 && idxDays100 <= 20)) {
            return ToString(d.Days()) + " " + GetLocalString(localizationId, "units.full.days(0)", "дней");
        } else if (idxDays == 1) {
            return ToString(d.Days()) + " " + GetLocalString(localizationId, "units.full.days(1)", "день");
        }
        return ToString(d.Days()) + " " + GetLocalString(localizationId, "units.full.days(2)", "дня");
    }
}

TString ILocalization::FormatPrice(const ELocalization& localizationId, const ui32 price, const std::initializer_list<TString>& units, TStringBuf separator) const {
    TStringBuilder result;
    result << ::ToString(price / 100);
    if (price % 10) {
        result << ',' << Sprintf("%02d", price % 100);
    } else if (price % 100) {
        result << ',' << Sprintf("%d", (price % 100) / 10);
    }
    if (units.size() > 0) {
        if (separator) {
            result << separator;
        } else {
            result << " ";
        }
    }
    for (auto&& i : units) {
        result << GetLocalString(localizationId, i, i);
    }
    return result;
}

TString ILocalization::DistanceFormatKm(const ELocalization localizationId, const double km, bool round) const {
    if (km < 0) {
        return "0 " + GetLocalString(localizationId, "units.short.km", "км");
    }

    if (km < 1) {
        return Sprintf("%d ", (int)(km * 1000)) + GetLocalString(localizationId, "units.short.meters", "м");
    } else if (round) {
        return Sprintf("%d %s", (int)std::round(km), GetLocalString(localizationId, "units.short.km", "км").c_str());
    } else {
        return Sprintf("%0.1f ", km) + GetLocalString(localizationId, "units.short.km", "км");
    }
}

TString ILocalization::FormatDuration(const ELocalization localizationId, const TDuration d, const bool withSeconds, const bool allowEmpty) const {
    const ui32 days = d.Days();
    const ui32 hours = d.Hours() % 24;
    const ui32 minutes = d.Minutes() % 60;
    const ui32 seconds = d.Seconds() % 60;
    TStringBuilder result;
    if (days) {
        result << days << " " << GetLocalString(localizationId, "units.short.day", "д");
    }
    if (hours) {
        if (!result.empty()) {
            result << ' ';
        }
        result << hours << " " << GetLocalString(localizationId, "units.short.hour", "ч");
    }
    if (minutes) {
        if (!result.empty()) {
            result << ' ';
        }
        result << minutes << " " << GetLocalString(localizationId, "units.short.minutes", "мин");
    }
    if (withSeconds && seconds) {
        if (!result.empty()) {
            result << ' ';
        }
        result << seconds << " " << GetLocalString(localizationId, "units.short.seconds", "с");
    }
    if (result.empty() && !allowEmpty) {
        result << d.Seconds() << " " << GetLocalString(localizationId, "units.short.seconds", "с");
    }
    return result;
}

TString ILocalization::FormatInstant(const ELocalization localizationId, const TInstant t) const {
    tm timeInfo;
    t.GmTime(&timeInfo);
    TStringBuilder sb;
    sb << ILocalization::FormatTimeOfTheDay(localizationId, t, TDuration::Zero()) << "\u00A0";
    sb << timeInfo.tm_mday << "\u00A0";
    switch (timeInfo.tm_mon) {
        case 0:
            return sb + GetLocalString(localizationId, "months.full.jan", "января");
        case 1:
            return sb + GetLocalString(localizationId, "months.full.feb", "февраля");
        case 2:
            return sb + GetLocalString(localizationId, "months.full.mar", "марта");
        case 3:
            return sb + GetLocalString(localizationId, "months.full.apr", "апреля");
        case 4:
            return sb + GetLocalString(localizationId, "months.full.may", "мая");
        case 5:
            return sb + GetLocalString(localizationId, "months.full.jun", "июня");
        case 6:
            return sb + GetLocalString(localizationId, "months.full.jul", "июля");
        case 7:
            return sb + GetLocalString(localizationId, "months.full.aug", "августа");
        case 8:
            return sb + GetLocalString(localizationId, "months.full.sep", "сентября");
        case 9:
            return sb + GetLocalString(localizationId, "months.full.oct", "октября");
        case 10:
            return sb + GetLocalString(localizationId, "months.full.nov", "ноября");
        case 11:
            return sb + GetLocalString(localizationId, "months.full.dec", "декабря");
        default:
            return sb;
    }
}

TString ILocalization::FormatInstantWithYear(const ELocalization localizationId, const TInstant t) const {
    tm timeInfo;
    t.GmTime(&timeInfo);
    TStringBuilder sb;
    sb << FormatInstant(localizationId, t) << "\u00A0" << (1900 + timeInfo.tm_year);
    return sb;
}

TString ILocalization::FormatTimeOfTheDay(const ELocalization localizationId, const TInstant timestamp, const TDuration shift) const {
    Y_UNUSED(localizationId);
    TInstant shifted = timestamp + shift;
    return shifted.FormatGmTime("%H:%M");
}

TString ILocalization::HoursFormat(const TDuration d, const TString& localizationId /*= "rus"*/) const {
    const ui32 idxHours = d.Hours() % 10;
    const ui32 idxHours100 = d.Hours() % 100;
    if (idxHours == 0 || idxHours >= 5 || (idxHours100 >= 10 && idxHours100 <= 20)) {
        return ToString(d.Hours()) + "\u00A0" + GetLocalString(localizationId, "units.full.hours(0)", "часов");
    } else if (idxHours == 1) {
        return ToString(d.Hours()) + "\u00A0" + GetLocalString(localizationId, "units.full.hours(1)", "час");
    }
    return ToString(d.Hours()) + "\u00A0" + GetLocalString(localizationId, "units.full.hours(2)", "часа");
}

TString ILocalization::MinutesFormat(const TDuration d, const TString& localizationId /*= "rus"*/) const {
    const ui32 idxMinutes = d.Minutes() % 10;
    const ui32 idxMinutes100 = d.Minutes() % 100;
    if (idxMinutes == 0 || idxMinutes >= 5 || (idxMinutes100 >= 10 && idxMinutes100 <= 20)) {
        return ToString(d.Minutes()) + "\u00A0" + GetLocalString(localizationId, "units.full.minutes(0)", "минут");
    } else if (idxMinutes == 1) {
        return ToString(d.Minutes()) + "\u00A0" + GetLocalString(localizationId, "units.full.minutes(1)", "минута");
    }
    return ToString(d.Minutes()) + "\u00A0" + GetLocalString(localizationId, "units.full.minutes(2)", "минуты");
}

TString ILocalization::FormatBonusRubles(const ELocalization localizationId, const ui32 amount, const bool withAmount) const {
    const ui32 idxAmount = amount % 10;
    const ui32 idxAmount100 = amount % 100;
    TString prefix = withAmount ? ToString(amount) + " " : "";
    if (idxAmount == 0 || idxAmount >= 5 || (idxAmount100 >= 10 && idxAmount100 <= 20)) {
        return prefix + GetLocalString(localizationId, "units.full.bonus_rubles(0)", "бонусных рублей");
    } else if (idxAmount == 1) {
        return prefix + GetLocalString(localizationId, "units.full.bonus_rubles(1)", "бонусный рубль");
    }
    return prefix + GetLocalString(localizationId, "units.full.bonus_rubles(2)", "бонусных рубля");
}

TString ILocalization::FormatFreeWaitTime(const ELocalization localizationId, const TDuration freeWaitTime) const {
    if (freeWaitTime == TDuration::Zero()) {
        return GetLocalString(localizationId, "offer_descriptions.elements.no_free_time", "Нет бесплатного ожидания");
    }
    return FormatDuration(localizationId, freeWaitTime) + GetLocalString(localizationId, "offer_descriptions.elements.of_waiting", " ожидания");
}

TString ILocalization::FormatDelegatedStandartOfferTitle(const ELocalization localizationId, const TString& offerName, const ui32 priceRiding) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.standart.short.header", "«‎_OfferName_»‎   ∙   _PriceRiding_ ₽/мин");
    SubstGlobal(pattern, "_OfferName_", offerName);
    SubstGlobal(pattern, "_PriceRiding_", FormatPrice(localizationId, priceRiding));
    return pattern;
}

TString ILocalization::FormatDelegatedStandartOfferBody(const ELocalization localizationId, const ui32 priceParking, const TDuration freeWaitTime) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.standart.short.body", "Ожидание — _PriceParking_ ₽/мин\n_FreeWaitTime_");
    SubstGlobal(pattern, "_PriceParking_", FormatPrice(localizationId, priceParking));
    SubstGlobal(pattern, "_FreeWaitTime_", FormatFreeWaitTime(localizationId, freeWaitTime));
    return pattern;
}

TString ILocalization::FormatDelegatedPackOfferTitle(const ELocalization localizationId, const TString& offerName) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.pack.short.header", "«‎_OfferName_»");
    SubstGlobal(pattern, "_OfferName_", offerName);
    return pattern;
}

TString ILocalization::FormatDelegatedPackOfferBody(const ELocalization localizationId, const ui32 distance, const TDuration duration, const ui32 priceParking, const TDuration freeWaitTime) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.pack.short.body", "_MileageLimit_   ∙   _PackDuration_\nОжидание — _PriceParking_ ₽/мин\n_FreeWaitTime_");
    SubstGlobal(pattern, "_MileageLimit_", ToString(distance) + " км");
    SubstGlobal(pattern, "_PackDuration_", FormatDuration(localizationId, duration));
    SubstGlobal(pattern, "_PriceParking_", FormatPrice(localizationId, priceParking));
    SubstGlobal(pattern, "_FreeWaitTime_", FormatFreeWaitTime(localizationId, freeWaitTime));
    return pattern;
}


TString ILocalization::FormatDelegationBodyWithPackOffer(const ELocalization localizationId, const TString& offerName, const ui32 remainingDistance, const TDuration remainingTime) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.pack.delegation.with_offer", "Тариф «‎_OfferName_»\n_MileageLimit_   ∙   _PackDuration_");
    SubstGlobal(pattern, "_OfferName_", offerName);
    SubstGlobal(pattern, "_MileageLimit_", ToString(remainingDistance) + " км");
    SubstGlobal(pattern, "_PackDuration_", FormatDuration(localizationId, remainingTime));
    return pattern;
}

TString ILocalization::FormatDelegationBodyWithoutPackOffer(const ELocalization localizationId, const ui32 remainingDistance, const TDuration remainingTime) const {
    TString pattern = GetLocalString(localizationId, "offer_descriptions.pack.delegation.without_offer", "Тогда _MileageLimit_ и _PackDuration_ сгорят");
    SubstGlobal(pattern, "_MileageLimit_", ToString(remainingDistance) + " км");
    SubstGlobal(pattern, "_PackDuration_", FormatDuration(localizationId, remainingTime));
    return pattern;
}

TString ILocalization::FormatIncomingDelegationMessage(const ELocalization localizationId, const TString& delegatorName, const TString& modelName) const {
    TString pattern = GetLocalString(localizationId, "delegation.p2p.incoming_push_message", "_DelegatorName_ желает передать вам управление _ModelName_");
    SubstGlobal(pattern, "_DelegatorName_", delegatorName);
    SubstGlobal(pattern, "_ModelName_", modelName);
    return pattern;
}

template <>
ELanguage enum_cast<ELanguage, ELocalization>(ELocalization value) {
    switch (value) {
    case ELocalization::Eng:    return LANG_ENG;
    case ELocalization::Rus:    return LANG_RUS;
    }
}
