#include <library/cpp/charset/codepage.h>
#include <library/cpp/mime/types/mime.h>
#include <library/cpp/on_disk/st_hash/static_hash_map.h>
#include <library/cpp/string_utils/url/url.h>

#include "attrconf.h"
#include "attrportion.h"
#include "attryndex.h"
#include "multilangdata.h"

#include <kernel/catfilter/hostattr.h>
#include <kernel/indexer/face/inserter.h>
#include <yweb/robot/dbscheeme/urlflags.h>
#include <ysite/yandex/common/urltok.h>
#include <ysite/yandex/common/stopurlpath.h>
#include <ysite/yandex/common/prepattr.h>
#include <util/string/reverse.h>
#include <util/string/cast.h>

template <class EqualKey>
struct TSthashIterator<ui32 const, const THostAttrsInfo, hash <ui32>, EqualKey> {
    typedef const THostAttrsInfo TValueType;
    typedef ui32 TKeyType;
    typedef EqualKey TKeyEqualType;
    typedef hash <ui32> THasherType;
    const char *Data;

    TSthashIterator()
        : Data(nullptr)
    {}
    TSthashIterator(const char *c)
        : Data(c)
    {}
    void operator++() {
        Data += GetLength();
    }
    bool operator != (const TSthashIterator &q) {
        return Data != q.Data;
    }
    bool operator == (const TSthashIterator &q) {
        return Data == q.Data;
    }
    ui32 Key() const {
        return * (ui32*) Data;
    }
    THostAttrsInfo Value() {
        const char *t = Data + sizeof(ui32), *d = t + sizeof(ui32);
        return THostAttrsInfo(*(ui32 *)t, (char *)d);
    }
    bool KeyEquals(const EqualKey &eq, const ui32 k) const {
        return eq(Key(), k);
    }
    size_t GetLength() const {
        size_t s = sizeof(ui32) + sizeof(ui32);
        return s + strlen(Data + s) + 1;
    }
};

class THostAttrsCache : public sthash_mapped<ui32, THostAttrsInfo, hash<ui32> > {
public:
    THostAttrsCache(const char* fname)
        : sthash_mapped<ui32, THostAttrsInfo, hash<ui32> >(fname, false)
    {
    }
};

TAttrPortion::TAttrPortion(const TAttrProcessorConfig* cfg, TMultilanguageDocData* multilangDocData)
    : ProcFlags(*cfg)
    , AttrStacker(new TAttrStacker())
    , MultilanguageDocData(multilangDocData)
{
    const TExistenceChecker& ec = cfg->ExistenceChecker;

    if (ec.Check(cfg->HostAttrsPath.data()))
        HostAttrsCache.Reset(new THostAttrsCache(cfg->HostAttrsPath.data()));

    if (NFs::Exists(cfg->TokenSplitterFile) && TokenSplitterData.Empty()) {
        TokenSplitterData = TBlob::FromFile(cfg->TokenSplitterFile.data());
    }
}

TAttrPortion::~TAttrPortion(){
}

static void StoreHostAttrs(TAttrStacker& attrer, const THostAttrsInfo& attrInfo) {
    char attrBuf[4096];
    ui32 ip = attrInfo.IP;
    if (ip) {
        sprintf(attrBuf, "%03d.%03d.%03d.%03d\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
        attrer.StoreDocAttrLit("hostip", attrBuf);
    }

    if (attrInfo.Whois == nullptr || strlen(attrInfo.Whois) == 0 || strlen(attrInfo.Whois) >= WHOIS_BUF_SIZE)
        return;

    strcat(strcpy(attrBuf, attrInfo.Whois), " ");
    char *whois = attrBuf, *space;
    while ((space = strchr(whois, ' ')) != nullptr) {
        *space = 0;
        attrer.StoreDocAttrLit("whois", whois);
        whois = space + 1;
    }
}

static void StoreUrlAttrs(
    TAttrStacker& attrer,
    const char *url,
    const TVector<ui32>& itdItpImagesMatchedFilters,
    const TVector<TString>& turboFeedTags,
    const TVector<TString>& siteRecommendationTags,
    const bool fromTurboFeed,
    const bool turboFeedKill,
    const bool siteRecommendationKill,
    const TString& zenSearchGroupingAttribute)
{
    TString host = TString{GetHost(url)};
    if (!host)
        return;
    if (host.find_first_not_of('.') == TString::npos)
        return;
    attrer.StoreDocAttrUrl("host", host.data());
    size_t i = 0;
    size_t j;
    TString rhost, domainPart;
    while (i < host.size()) {
        j = host.find('.', i);
        if (j == TString::npos)
            j = host.size();
        if (j > i) {
             domainPart = TString(host.substr(i, j - i));
             attrer.StoreDocAttrUrl("domain", domainPart.data());
             ReverseInPlace(domainPart);
             if (rhost.size())
                 rhost += ".";
             rhost += domainPart;
        }
        i = j + 1;
    }
    ReverseInPlace(rhost);
    attrer.StoreDocAttrUrl("domain", "root");
    attrer.StoreDocAttrUrl("rhost", rhost.data());
    if (!turboFeedKill && !siteRecommendationKill) {
        static constexpr TStringBuf Delimiter = "::";

        for (ui32 filterId : itdItpImagesMatchedFilters) {
            for (const TString& turboTag : turboFeedTags) {
                const TString filterTagRhostValue = TString::Join(ToString(filterId), Delimiter, turboTag, Delimiter, rhost);
                attrer.StoreDocAttrUrl("itditp_turbofiltertagrhost", filterTagRhostValue.data());
            }

            for (const TString& tag : siteRecommendationTags) {
                const TString filterTagRhostValue = TString::Join(ToString(filterId), Delimiter, tag, Delimiter, rhost);
                attrer.StoreDocAttrUrl("itditp_filtertagrhost", filterTagRhostValue.data());
            }

            const TString filterRhostValue = TString::Join(ToString(filterId), Delimiter, rhost);

            attrer.StoreDocAttrUrl("itditp_filter_and_rhost", filterRhostValue.Data());

            if (fromTurboFeed) {
                attrer.StoreDocAttrUrl("itditp_turbo_filter_and_rhost", filterRhostValue.Data());
            }
        }

        for (const TString& turboTag : turboFeedTags) {
            const TString filterTagRhostValue = TString::Join(turboTag, Delimiter, rhost);
            attrer.StoreDocAttrUrl("itditp_turbo_tag_and_rhost", filterTagRhostValue.data());
        }

        for (const TString& tag : siteRecommendationTags) {
            const TString filterTagRhostValue = TString::Join(tag, Delimiter, rhost);
            attrer.StoreDocAttrUrl("itditp_tag_and_rhost", filterTagRhostValue.data());
        }

        if (fromTurboFeed) {
            attrer.StoreDocAttrUrl("itditp_turbo_rhost", rhost.data());
        }
    }
    if (!zenSearchGroupingAttribute.empty()) {
        attrer.StoreDocAttrUrl("zen_grouping_attr", zenSearchGroupingAttribute.data());
    }
}

static void SplitterAttrs(TAttrStacker& attrer, const char *url) {
    TURLTokenizer tokenizer(url, strlen(url), false, false, WORD_LEVEL_Max);
    const size_t count = tokenizer.GetTokenCount();
    for (size_t i = 0; i < count; ++i) {
        const TURLTokenizer::TToken& token = tokenizer.GetToken(i);
        attrer.StoreDocAttrUrl("upath", token.data());
        TUtf16String upath(token.data(), token.size());
        upath.to_lower(); // required for IsStopPathComponent()
        TUtf16String b1, b2;
        if (IsStopPathComponent(WideToUTF8(upath).c_str()) && count > 1)
            CreatePathComponentBigrams(tokenizer, i, b1, b2);
        if (!b1.empty()) {
            attrer.StoreDocAttrUrl("lupath", b1.data());
        } else {
            attrer.MoveAttrPos("lupath");
        }
        if (!b2.empty()) {
            attrer.StoreDocAttrUrl("rupath", b2.data());
        } else {
            attrer.MoveAttrPos("rupath");
        }
    }
}

void TAttrPortion::ProcessSearchAttr(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo,
    const TAttrProcessorFlags* flags) const {
    if (!flags) {
        flags = &ProcFlags;
    }

    AttrStacker->SetInserter(inserter);

    if (docInfo.DocHeader->MimeType != MIME_HTML && docInfo.DocHeader->MimeType != MIME_XML && docInfo.DocHeader->MimeType < MIME_MAX)
        AttrStacker->StoreDocAttrLit("mime", MimeNames[docInfo.DocHeader->MimeType]);
    if (docInfo.ModTime && !flags->IgnoreDateAttrs)
        AttrStacker->StoreDateTime("date", docInfo.ModTime);
    if (docInfo.DocHeader->Flags & UrlFlags::NEWDOC)
        AttrStacker->StoreDocAttrLit("new", "1");
    if (docInfo.IsPPBHost)
        AttrStacker->StoreDocAttrLit("ppbhost", "1");
    if (docInfo.IsMobileUrl)
        AttrStacker->StoreDocAttrLit("mobile", "1");
    if (docInfo.NeverReachableFromMorda) {
        AttrStacker->StoreDocAttrLit("never_reachable_from_morda", "1");
    }
    if (docInfo.IsTurboEcom) {
        AttrStacker->StoreDocAttrLit("is_turbo_ecom", "1");
    }
    if (docInfo.IsTopClickedCommHost) {
        AttrStacker->StoreDocAttrLit("is_top_clicked_comm_host", "1");
    }
    if (docInfo.IsYANHost) {
        AttrStacker->StoreDocAttrLit("is_yan_host", "1");
    }
    if (docInfo.IsFilterExp1) {
        AttrStacker->StoreDocAttrLit("is_filter_exp1", "1");
    }
    if (docInfo.IsFilterExp2) {
        AttrStacker->StoreDocAttrLit("is_filter_exp2", "1");
    }
    if (docInfo.IsFilterExp3) {
        AttrStacker->StoreDocAttrLit("is_filter_exp3", "1");
    }
    if (docInfo.IsFilterExp4) {
        AttrStacker->StoreDocAttrLit("is_filter_exp4", "1");
    }
    if (docInfo.IsFilterExp5) {
        AttrStacker->StoreDocAttrLit("is_filter_exp5", "1");
    }
    for (size_t yabsFilterIdx = 0; yabsFilterIdx < docInfo.YabsFlags.size(); ++yabsFilterIdx) {
        if (docInfo.YabsFlags[yabsFilterIdx]) {
            const TString filterName = "is_doc_direct_vertical_filter" + ToString(yabsFilterIdx);
            AttrStacker->StoreDocAttrLit(filterName.data(), "1");
        }
    }

    const char* url = docInfo.DocHeader->Url;

    size_t schemeSize;
    if (flags->CutScheeme) { // any scheme (yandex.server)
        const char *p = strstr(url, "://");
        schemeSize = p ? (p + 3 - url) : 0;
    } else { // only known schemes (regular robot)
        schemeSize = GetHttpPrefixSize(url);
    }

    if (schemeSize) {
        TString scheme(url, schemeSize - 3);
        if (scheme != "http")
            AttrStacker->StoreDocAttrLit("scheme", scheme.data());
        url += schemeSize;
    }

    const size_t bufSize = 2 * FULLURL_MAX;
    char buf[bufSize];
    ECharset e = (ECharset)docInfo.DocHeader->Encoding;
    if (SingleByteCodepage(e)) {
        size_t len = FixNationalUrl(url, buf, bufSize - 1, e);
        buf[len] = 0;
        url = buf;
    }

    AttrStacker->StoreDocAttrUrl("url", url);
    if (flags->IndexUrlAttributes) {
        StoreUrlAttrs(
            *AttrStacker,
            url,
            docInfo.ItdItpImagesMatchedFilters,
            docInfo.TurboFeedTags,
            docInfo.SiteRecommendationTags,
            docInfo.FromTurboFeed,
            docInfo.TurboFeedKill,
            docInfo.SiteRecommendationKill,
            docInfo.ZenSearchGroupingAttribute);
    }

    if (flags->IndexUrl) {
        AttrStacker->StoreUrl(url, static_cast<ELanguage>(docInfo.DocHeader->Language), docInfo.UrlTransliterationData, &TokenSplitterData);
    }

    if (flags->SplitUrl) {
        SplitterAttrs(*AttrStacker.Get(), url);
        if (docInfo.DocHeader->Language > 0 && docInfo.DocHeader->Language < LANG_MAX) //!!!! It can't be 0, but it happens in url.dat for a couple of urls
        {
            const char* langName = IsoNameByLanguage(static_cast<ELanguage>(docInfo.DocHeader->Language));
            if (!langName || !*langName)   // IsoNameByLanguage can return NULL
                langName = NameByLanguage(static_cast<ELanguage>(docInfo.DocHeader->Language));
            AttrStacker->StoreDocAttrLit("lang", langName);
        }
    }

    if (MultilanguageDocData)
        MultilanguageDocData->StoreMultilanguageAttr(*inserter, docInfo.DocId);

    if (!!HostAttrsCache) {
        sthash<ui32, THostAttrsInfo, hash<ui32> >::const_iterator it = (*HostAttrsCache.Get())->find(docInfo.HostId);
        if (it != (*HostAttrsCache.Get())->end()) {
            THostAttrsInfo attrInfo = it.Value();
            StoreHostAttrs(*AttrStacker.Get(), attrInfo);
        }
    }

    AttrStacker->Clear();
}
