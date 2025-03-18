#pragma once

#include <tools/snipmake/common/common.h>

#include <library/cpp/json/json_value.h>

#include <util/stream/input.h>

namespace NSnippets {

namespace NJsonFields {
    static const char TITLE[] = "title";
    static const char HEADLINE_SRC[] = "headline_src";
    static const char HEADLINE[] = "headline";
    static const char BY_LINK[] = "by_link";
    static const char LINES[] = "lines";
    static const char PASSAGES[] = "passages";
    static const char SPEC_ATTRS[] = "spec_attrs";
    static const char REPORT_ATTRS[] = "report_attrs";
    static const char PREVIEW_JSON[] = "preview_json";
    static const char EXPL[] = "expl";
    static const char DOC[] = "doc";
    static const char REQ[] = "req";
    static const char USERREQ[] = "userreq";
    static const char REGION[] = "region";
    static const char URL[] = "url";
    static const char HILITEDURL[] = "hilitedurl";
    static const char URLMENU[] = "urlmenu";
    static const char MARKET[] = "market";
    static const char SNIP_WIDTH[] = "snip_width";
    static const char CLICKLIKE_JSON[] = "clicklike_json";
    static const char IMG[] = "img";
    static const char IMG_AVATARS[] = "img_avatars";
    static const char LINK_ATTRS[] = "link_attrs";
}

class TSnippetsJSONIterator : public ISnippetsIterator {
public:
    TSnippetsJSONIterator(IInputStream* inp, bool isRCA);
    ~TSnippetsJSONIterator() override;
    bool Next() override;
    const TReqSnip& Get() const override;
private:
    bool ReadSnippet(const NJson::TJsonValue& jsonValue);
    bool ValidateSnippet(const NJson::TJsonValue& jsonValue) const;
private:
    IInputStream* Input;
    TReqSnip CurrentSnip;
    bool IsRCA = false;
};

} // NSnippets
