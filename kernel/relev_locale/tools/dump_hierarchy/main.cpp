#include <kernel/relev_locale/relev_locale.h>

#include <kernel/country_data/countries.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/langmask/langmask.h>

#include <util/stream/output.h>

NJson::TJsonValue GetLocalesData(NRl::ERelevLocale locale) {
    NJson::TJsonValue data(NJson::JSON_MAP);
    data["name"] = ToString(locale);
    const TCountryData* countryData = GetCountryData(RelevLocale2Country(locale));
    if (countryData) {
        data["geo-code"] = countryData->CountryId;
        data["erf-code"] = countryData->ErfCode;
        data["tld"] = countryData->NationalDomain;
        data["lang"] = NLanguageMasks::ToString(countryData->NationalLanguage);
    }
    NJson::TJsonValue childNodes(NJson::JSON_ARRAY);
    for (auto chLocale: NRl::GetLocaleChildren(locale)) {
        childNodes.AppendValue(GetLocalesData(chLocale));
    }
    if (childNodes.GetArray().size()) {
        data["children"] = std::move(childNodes);
    }
    return data;
}

int main() {
    NJson::TJsonValue locales = GetLocalesData(NRl::RL_UNIVERSE);
    WriteJson(&Cout, &locales, true);
    return 0;
}
