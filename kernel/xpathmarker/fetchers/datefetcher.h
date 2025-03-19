#pragma once

#include "attribute_fetcher.h"

#include <kernel/xpathmarker/xmlwalk/sanitize.h>
#include <kernel/xpathmarker/xmlwalk/utils.h>
#include <kernel/xpathmarker/utils/debug.h>

#include <kernel/dater/dater_simple.h>
#include <kernel/dater/convert_old/old_dater.h>

namespace NHtmlXPath {

class TDateAttributeFetcher : public IAttributeValueFetcher {
public:
    TDateAttributeFetcher(ELanguage language = LANG_UNK) {
        ELanguage daterLanguage =
            (language != LANG_UNK && language != LANG_EMPTY)
            ? language
            : LANG_RUS;
        Dater.Localize(daterLanguage);
        SettedLanguage = daterLanguage;
        NeededLanguage = daterLanguage;
    }

    bool CanFetch(EAttributeType type) const override {
        return type == AT_DATE;
    }

    void Fetch(const NXml::TxmlXPathObjectPtr& attributeData, const NJson::TJsonValue& attributeMetadata, TAttributes& attributes) override {
        if (NeededLanguage != SettedLanguage) {
            XPATHMARKER_INFO("Switching TDater language to " << NameByLanguage(NeededLanguage) << " from " << NameByLanguage(SettedLanguage))

            Dater.Localize(NeededLanguage);
            SettedLanguage = NeededLanguage;
        }

        TString text = SerializeToStroka(attributeData);
        CompressWhiteSpace(text);
        DecodeHTMLEntities(text);

        XPATHMARKER_INFO("TDateAttributeFetcher got raw date " << text)
        attributes.push_back(TAttribute(attributeMetadata[NAME_ATTRIBUTE].GetStringRobust() + RAW_DATE_ATTR_POSTFIX, text, AT_DATE));

        TUtf16String wideText;
        UTF8ToWide(text, wideText);

        Dater.InputText = wideText;
        Dater.Scan();

        if (!Dater.OutputDates.empty()) {
            ND2::TDate date = Dater.OutputDates[0];
            NDater::TDaterDate normalizedDate(
                date.Data.Year, date.Data.Month, date.Data.Day,
                NDater::TDaterDate::FromText, date.Features.Has4DigitsInYear, date.Features.MonthIsWord
            );

            TTempBuf dateAttr(NDater::TDaterDate::INDEX_ATTR_BUFSIZE);
            normalizedDate.ToIndexAttr(dateAttr.Data());

            XPATHMARKER_INFO("TDateAttributeFetcher got date '" << dateAttr.Data() << "' from '" << text << "'")
            attributes.push_back(TAttribute(attributeMetadata[NAME_ATTRIBUTE].GetStringRobust() + NORMALIZED_DATE_ATTR_POSTFIX, dateAttr.Data(), AT_DATE));

            Dater.Clear();
            SettedLanguage = LANG_UNK;
        } else {
            XPATHMARKER_INFO("TDateAttributeFetcher can't find date in '" << text << "'")
        }
    }

    void Reset(ELanguage language) {
         ELanguage daterLanguage =
            (language != LANG_UNK && language != LANG_EMPTY)
            ? language
            : LANG_RUS;
         NeededLanguage = daterLanguage;
    }

    ~TDateAttributeFetcher() override {
    }
private:
    ND2::TDater Dater;

    // For lazy TDater localization (due to perfomance troubles)
    ELanguage SettedLanguage;
    ELanguage NeededLanguage;

    static const char* NORMALIZED_DATE_ATTR_POSTFIX;
    static const char* RAW_DATE_ATTR_POSTFIX;
};


} // namespace NHtmlXPath
