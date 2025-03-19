#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

class TNewsOrangeAnnotation {
public:
    TInstant Date;
    TString Title;
    TString Snippet;
    float AgencyRate;
    size_t Count;
    TVector<std::pair<TString, TString> > Urls;

    TNewsOrangeAnnotation() {
    }

    TString Serialize();
    void Parse(const TString& rawData);
    void Clear();
    bool IsEmpty();
};
