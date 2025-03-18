#pragma once

#include <library/cpp/uri/http_url.h>

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/face/zoneconf.h>

class IParsedDocProperties;
class THtConfigurator;

class TAttributeExtractor: public IZoneAttrConf {
public:
    explicit TAttributeExtractor(const THtConfigurator* config, bool collectLinks = true);

    void Reset();

    void CheckEvent(const THtmlChunk& evnt, IParsedDocProperties* docProp, TZoneEntry* result) override;

private:
    void CheckEventImpl(const THtmlChunk& evnt, IParsedDocProperties* docProp, TZoneEntry* result);

private:
    const THtConfigurator* const Configuration_;

    THttpURL HtBase_;
    const bool CollectLinks_; // put links to IParsedDocProperties, PP_LINKS_HASH
    bool Follow_;
};
