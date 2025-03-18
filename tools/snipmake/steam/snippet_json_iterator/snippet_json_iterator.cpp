#include "snippet_json_iterator.h"

#include <library/cpp/json/json_reader.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/stream/str.h>

namespace NSnippets {

TSnippetsJSONIterator::TSnippetsJSONIterator(IInputStream* inp, bool isRCA)
    : Input(inp)
    , IsRCA(isRCA)
{ }

TSnippetsJSONIterator::~TSnippetsJSONIterator()
{ }

bool TSnippetsJSONIterator::Next() {
    CurrentSnip = TReqSnip();

    TString jsonStr;
    bool gotSnippet = false;
    while (!gotSnippet && Input->ReadLine(jsonStr)) {
        TStringInput inputJsonStr(jsonStr);
        NJson::TJsonValue jsonValue;
        if (NJson::ReadJsonTree(&inputJsonStr, &jsonValue)) {
            if (ReadSnippet(jsonValue)) {
                gotSnippet = true;
            } else {
                Cerr << "TSnippetsJSONIterator::Next: incomplete snippet: \"" <<
                    jsonStr << "\"" << Endl;
            }
        } else {
            Cerr << "TSnippetsJSONIterator::Next: can't read JSON: \"" <<
                jsonStr << "\"" << Endl;
        }
    }
    return gotSnippet;
}

const TReqSnip& TSnippetsJSONIterator::Get() const {
    return CurrentSnip;
}

bool TSnippetsJSONIterator::ValidateSnippet(const NJson::TJsonValue& jsonValue) const {
    if (IsRCA) {
        return jsonValue.Has(NJsonFields::URL) && (jsonValue.Has(NJsonFields::TITLE) || jsonValue.Has(NJsonFields::PASSAGES));
    } else {
        return jsonValue.Has(NJsonFields::USERREQ) &&
               jsonValue.Has(NJsonFields::URL) &&
               jsonValue.Has(NJsonFields::REGION) &&
               jsonValue.Has(NJsonFields::TITLE);
    }

}

bool TSnippetsJSONIterator::ReadSnippet(const NJson::TJsonValue& jsonValue) {
    if (!ValidateSnippet(jsonValue)) {
        return false;
    }
    if (IsRCA) {
        // Query used in names of tasks in interface.
        // But RCA doesn't have query, this have a url
        CurrentSnip.Query = jsonValue[NJsonFields::URL].GetString();
    } else {
        CurrentSnip.Query = jsonValue[NJsonFields::USERREQ].GetString();
    }

    CurrentSnip.Url = UTF8ToWide(jsonValue[NJsonFields::URL].GetString());
    CurrentSnip.HilitedUrl = UTF8ToWide(jsonValue[NJsonFields::HILITEDURL].GetString());
    CurrentSnip.Region = jsonValue[NJsonFields::REGION].GetString();
    CurrentSnip.TitleText = UTF8ToWide(jsonValue[NJsonFields::TITLE].GetString());
    CurrentSnip.Headline = UTF8ToWide(jsonValue[NJsonFields::HEADLINE].GetString());
    CurrentSnip.HeadlineSrc = jsonValue[NJsonFields::HEADLINE_SRC].GetString();
    CurrentSnip.Lines = jsonValue[NJsonFields::LINES].GetString();
    const NJson::TJsonValue::TArray& fragments = jsonValue[NJsonFields::PASSAGES].GetArray();
    CurrentSnip.SnipText.reserve(fragments.size());
    for (size_t i = 0; i < fragments.size(); ++i) {
        CurrentSnip.SnipText.push_back(TSnipFragment(UTF8ToWide(fragments[i].GetString())));
    }

    NJson::TJsonValue extraJsonValue(NJson::JSON_MAP);
    if (jsonValue[NJsonFields::BY_LINK].GetBoolean()) {
        extraJsonValue[NJsonFields::BY_LINK] = jsonValue[NJsonFields::BY_LINK];
    }
    if (jsonValue.Has(NJsonFields::URLMENU)) {
        extraJsonValue[NJsonFields::URLMENU] = jsonValue[NJsonFields::URLMENU];
    }
    if (jsonValue.Has(NJsonFields::MARKET)) {
        extraJsonValue[NJsonFields::MARKET] = jsonValue[NJsonFields::MARKET];
    }
    if (jsonValue.Has(NJsonFields::SPEC_ATTRS)) {
        extraJsonValue[NJsonFields::SPEC_ATTRS] = jsonValue[NJsonFields::SPEC_ATTRS];
    }
    if (jsonValue.Has(NJsonFields::IMG_AVATARS) && jsonValue.Has(NJsonFields::IMG)) {
        extraJsonValue[NJsonFields::IMG] = jsonValue[NJsonFields::IMG];
        extraJsonValue[NJsonFields::IMG_AVATARS] = jsonValue[NJsonFields::IMG_AVATARS];
    }
    if (jsonValue.Has(NJsonFields::LINK_ATTRS)) {
        extraJsonValue[NJsonFields::LINK_ATTRS] = jsonValue[NJsonFields::LINK_ATTRS];
    }
    if (!extraJsonValue.GetMap().empty()) {
        CurrentSnip.ExtraInfo = ToString(extraJsonValue);
    }

    return true;
}

} // NSnippets
