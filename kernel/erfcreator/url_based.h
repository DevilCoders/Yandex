#pragma once

#include "config.h"
#include "erfcreator.h"
#include <util/generic/ptr.h>

class IUrlHandler;
typedef TVector<TSimpleSharedPtr<IUrlHandler> > THandlers;

class TErfUrlFiller : TNonCopyable {
private:
    const TErfCreateConfig& Config;
    TErfsRemap& Erfs;
    THandlers Handlers;
    THandlers HandlersAll;

public:
    TErfUrlFiller(const TErfCreateConfig& config, TErfsRemap& erfs);
    ~TErfUrlFiller();
    void ProcessUrls(void);
};

class TRealtimeErfUrlFiller : TNonCopyable {
private:
    THandlers Handlers;
public:
    TRealtimeErfUrlFiller();
    ~TRealtimeErfUrlFiller();
    void ProcessUrl(const TString& url, SDocErfInfo3& erf);
};
