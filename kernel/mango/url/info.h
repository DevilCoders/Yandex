#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <library/cpp/uri/http_url.h>
#include <util/string/vector.h>

namespace NMango {

class TUrlInfo
{
public:
    TUrlInfo() : Valid(false) {}
    TUrlInfo(const TString &url, const TString &baseUrl = "");
    TUrlInfo(const THttpURL &url);

    bool IsValid() const { return Valid; }
    const THttpURL& GetParsed() const { return Url; }
    TString GetRaw() const;
    TString GetCanonized() const;
    TStringBuf GetHost() const;
    TStringBuf GetPath() const;
    ui32   GetOutcomingHash() const;
    static inline ui32 GetOutcomingHash32(ui64 erfHash)
    {
        return 1 + erfHash % static_cast<ui64>(4294967291LL); // magic prime number
    }

    bool   IsDeprecated() const;
    bool   IsTrash() const;
    bool   IsMorda() const;
    bool   IsCrawledByPPB() const;
    bool   IsUrlMangler() const;
    bool   IsTiny() const;
    bool   IsImage() const;
    bool   IsYoutubeVideo() const;
    bool   NeedToDownload() const;

    bool IsIn(const TVector<TString> &list) const;
    bool IsIn(const TVector< std::pair<TString, TString> > &blackList) const;
private:
    THttpURL Url;
    bool Valid;
};

void FixHttpScheme(TString& url);

} // namespace NMango
