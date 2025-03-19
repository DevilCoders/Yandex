#include "check.h"

#include <kernel/urlnorm/urlnorm.h>
#include <kernel/urlnorm/normalize.h>
#include <kernel/searchlog/errorlog.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/wide.h>
#include <util/charset/unidata.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/list.h>
#include <util/folder/dirut.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/strip.h>
#include <util/charset/utf8.h>
#include <util/stream/output.h>
#include <util/stream/format.h>
#include <util/system/maxlen.h>

using namespace NDups;
const ui32 TUrlRegionsInfo::UnknownRegion = (ui32)-1;

NDups::EDupMode NDups::GetDupModeFromInt(int mode) {
    switch(mode) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            return Signatures;
        case 0:
        default:
            return UrlOnly;
    }
}

void SeparateHostPath(const TStringBuf& docUrl, TString& host, TString& path, bool normalize) {
    TString normalizedUrl;
    if (!normalize || !NUrlNorm::NormalizeUrl(docUrl, normalizedUrl))
        normalizedUrl = docUrl;

    TStringBuf url(normalizedUrl);
    url.Skip(GetHttpPrefixSize(url));

    size_t slash = url.find('/');
    host = url.Head(slash);
    host.to_lower();

    path = "/";
    if (slash != TStringBuf::npos) {
        path = TString{url.Tail(slash)};
        UrlUnescape(path);

        // convert encoding (especially useful for wikipedia urls)
        if (!IsUtf(path))
            path = WideToUTF8(CharToWide(path, CODES_WIN));
        // remove trailing slash but not the last one
        while (path.size()>1 && path.back() == '/')
            path.pop_back();
    }
}

ui32 NDups::GetHashKeyForUrl(const TStringBuf& dataUrl, bool normalize, ui32* hostKey) {
    TStringBuf url(dataUrl);
    TString host, path;
    SeparateHostPath(url, host, path, normalize);
    ui64 pathHash = 0;
    ui32 key = FnvHash<ui32>(host.data(), host.size());
    if (hostKey) {
        *hostKey = key;
    }
    if (path.size() < URL_MAX && !UrlHashVal(pathHash, path.c_str(), false)) {
        key = FnvHash<ui32>(reinterpret_cast<const ui8*>(&pathHash), sizeof(ui64), key);
    } else {
        key = FnvHash<ui32>(path.data(), path.size(), key);
    }
    return key;
}

namespace {
    void FillWhiteList(THashSet<TString>& list, const TString& dirName, const TString& whiteLists) {
        TVector<TString> whiteFiles = SplitString(StripString(whiteLists), "|");
        for (auto wlfile : whiteFiles) {
            if (NFs::Exists(dirName + wlfile)) {
                try {
                    TFileInput whiteListFile(dirName + wlfile);
                    TString hostName;
                    while (whiteListFile.ReadLine(hostName)) {
                        list.insert(StripString(hostName));
                    }
                } catch (const TIoException& e) {
                    Cerr << "Problems with antidup white list file '" << wlfile << "' : " << e.what() << Endl;
                }
            }
        }
    }
}

TAntidupWhiteList::TAntidupWhiteList(const TString& dirName, const TString& whiteLists, const TString& whiteUrlLists) {
    FillWhiteList(Hosts, dirName, whiteLists);
    FillWhiteList(Urls, dirName, whiteUrlLists);
}

bool TAntidupWhiteList::IsDocInWhiteList(const TStringBuf& docUrl) const {
    TString hostPart, pathPart;
    SeparateHostPath(docUrl, hostPart, pathPart, false);
    const bool hostFound = (Hosts.find(hostPart) != Hosts.end());
    const bool urlFound = (Urls.find(docUrl) != Urls.end());
    return hostFound || urlFound;
}

namespace NDups {
    TCheckedSourceFrequencies::TCheckedSourceFrequencies()
        : PerSourceCount{{0}}
    {}

    void TCheckedSourceFrequencies::AddOne(ECheckedSourceType st) {
        auto ind = static_cast<size_t>(st);
        if (ind < PerSourceCount.size())
            PerSourceCount[ind]++;
    }

    size_t TCheckedSourceFrequencies::GetMostFrequentCheckedSourceCount() const {
        //skip 'NonChecked' element
        auto el = std::max_element(PerSourceCount.begin() + static_cast<size_t>(ECheckedSourceType::NonChecked) + 1, PerSourceCount.end());
        return *el;
    }
};
