#include "region_name.h"

#include <kernel/country_data/countries.h>

#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/string/strip.h>
#include <util/stream/file.h>
#include <util/generic/map.h>
#include <library/cpp/langs/langs.h>

static const TString EMPTY_STRING;

void TRegionNameData::ReadLrToName(const TString& fileName) {
    TFileInput fIn(fileName);
    TString line;
    size_t lineInd = 0;
    while (fIn.ReadLine(line)) {
        const static TSplitDelimiters DELIMS("\t");
        TDelimitersStrictSplit splitter(line, DELIMS);
        TDelimitersStrictSplit::TIterator it = splitter.Iterator();
        TCateg lr = FromString<TCateg>(it.NextString());
        TLrName lrName;
        size_t ind = 0;
        for (; ind < lrName.Name.size() && !it.Eof(); ++ind) {
            lrName.Name[ind] = Strip(it.NextString());
            //"-" means no name defined in geobase
            if (lrName.Name[ind] == "-") {
                lrName.Name[ind] = "";
            }
        }
        if (lrName.Name.size() != ind) {
            ythrow yexception() << "Error in TRegionNameRule::ReadLrToName: 'lr2name.txt line " << lineInd + 1
                                << " format is incorrect'";
        }
        LrToName.insert(TLrToName::value_type(lr, lrName));
        ++lineInd;
    }
}


namespace {
    struct TWrapper {
        //! Note that order is correlating with order of names in TLrName::Name
        using TLangToLang = TMap<ELanguage, size_t>;
        using TCountryToLang = TMap<TCateg, size_t>;

        TLangToLang LangToLang;
        TCountryToLang CountryToLang;

        TWrapper() {
            LangToLang[LANG_RUS] = TRegionNameData::TLrName::Russian;
            LangToLang[LANG_UKR] = TRegionNameData::TLrName::Ukrainian;
            LangToLang[LANG_KAZ] = TRegionNameData::TLrName::Kazakh;
            LangToLang[LANG_BEL] = TRegionNameData::TLrName::Belarussian;
            LangToLang[LANG_TAT] = TRegionNameData::TLrName::Tatar;
            LangToLang[LANG_TUR] = TRegionNameData::TLrName::Turkish;

            CountryToLang[COUNTRY_RUSSIA.CountryId] = TRegionNameData::TLrName::Russian;
            CountryToLang[COUNTRY_UKRAINE.CountryId] = TRegionNameData::TLrName::Russian;
            CountryToLang[COUNTRY_KAZAKHSTAN.CountryId] = TRegionNameData::TLrName::Russian;
            CountryToLang[COUNTRY_BELARUS.CountryId] = TRegionNameData::TLrName::Russian;
            CountryToLang[COUNTRY_TURKEY.CountryId] = TRegionNameData::TLrName::Turkish;
        }
    };
}

const TString& TRegionNameData::GetLrName(const TCateg lr, const TCateg countryId,
                                         const TLangMask& langMask) const
{
    const TLrToName::const_iterator itLrName = LrToName.find(lr);
    if (itLrName == LrToName.end()) {
        return EMPTY_STRING;
    }
    const TLrName& lrNameBundle = itLrName->second;
    const TWrapper* wr = Singleton<TWrapper>();

    const TString* lrName = &EMPTY_STRING;
    for (const auto& langToLang: wr->LangToLang) {
        if (langMask.Test(langToLang.first)) {
            lrName = &lrNameBundle.Name[langToLang.second];
            break;
        }
    }
    if (lrName->empty() && END_CATEG != countryId) {
        TWrapper::TCountryToLang::const_iterator it = wr->CountryToLang.find(countryId);
        if (it != wr->CountryToLang.end()) {
            lrName = &lrNameBundle.Name[it->second];
        }
    }
    if (lrName->empty() && langMask.Test(LANG_ENG)) {
        lrName = &lrNameBundle.Name[TLrName::English];
    }
    // It is assumend that russian name is always available
    if (lrName->empty()) {
        lrName = &lrNameBundle.Name[TLrName::Russian];
    }

    return *lrName;
}
