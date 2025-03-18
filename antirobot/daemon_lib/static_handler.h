#pragma once

#include "selective_handler.h"

#include <library/cpp/http/static/static.h>
#include <util/generic/singleton.h>

namespace NAntiRobot {

class TStaticData : public CHttpServerStatic {
private:
    TVector<TString> Docs;
    TSelectiveHandler<TUrlLocationSelector> Handler;

    void AddDoc(const TString& key, const char* webPath, const char* mimeType);
    void AddDocWithHandler(const TString& key, const char* webPath, const char* mimeType);

public:
    TStaticData();

    static TStaticData& Instance() {
        return *Singleton<TStaticData>();
    }

    TSelectiveHandler<TUrlLocationSelector> GetHandler() const {
        return Handler;
    }
};

} // namespace NAntiRobot
