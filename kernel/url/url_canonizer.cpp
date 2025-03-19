#include "url_canonizer.h"

#include <kernel/hosts/owner/owner.h>
#include <kernel/hosts/minifilter/minifilter.h>
#include <library/cpp/charset/codepage.h>
#include <util/generic/yexception.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/string_utils/old_url_normalize/url.h>
#include <library/cpp/tld/tld.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/string/vector.h>
#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <contrib/libs/libidn/lib/idna.h>
#include <util/string/split.h>

bool IHostCanonizer::IsResolvingMirrors() const {
    return false;
}

TMirrorResolver::TMirrorResolver(const TString& mirrorsFilename) {
    Mirrors.Reset(new mirrors());
    Mirrors->load(mirrorsFilename.data());
}

TString TMirrorResolver::GetMainMirror(const TString& host) const {
    const char* mainMirror = Mirrors->check(host.data());
    if (mainMirror == nullptr) {
        return host;
    } else {
        return mainMirror;
    }
}

TMirrorMappedResolver::TMirrorMappedResolver(const TString& mirrorsMappedFilename) {
    Mirrors.Reset(new mirrors_mapped(mirrorsMappedFilename.data()));
}

TString TMirrorMappedResolver::GetMainMirror(const TString& host) const {
    const char* mainMirror = Mirrors->check(host.data());
    if (mainMirror == nullptr) {
        return host;
    } else {
        return mainMirror;
    }
}

TOwnerResolver::TOwnerResolver(const TString& ownersFilename) {
    Owners.Reset(new hostset());
    OwnersPool.Reset(new segmented_string_pool());
    Owners->load(ownersFilename.data(), *OwnersPool);
}

TOwnerResolver::~TOwnerResolver() {
}

TString TOwnerResolver::GetOwner(const TString& host) const {
    return GetHostOwner(*Owners, host.data());
}

TCombinedHostCanonizer::TCombinedHostCanonizer()
    : CurrentFillingStage(FS_BEFORE_MIRRORING)
    , IsSmartAdding(false)
    , IsEmpty(true)
{
}

TCombinedHostCanonizer::TCombinedHostCanonizer(const THostCanonizerPtr& canonizer)
    : CurrentFillingStage(FS_BEFORE_MIRRORING)
    , IsSmartAdding(false)
    , IsEmpty(true)
{
    AddCanonizer(canonizer);
}

void TCombinedHostCanonizer::AddBeforeMirroringCanonizer(const THostCanonizerPtr& canonizer) {
    if (IsSmartAdding) {
        ythrow yexception() << "Implicit and explicit ways to add canonizer to TCombinedHostCanonizer are mixed";
    }
    if (canonizer->IsResolvingMirrors()) {
        ythrow yexception() << "Canonizer in stage 'before mirroring' must not resolve mirrors";
    }
    IsEmpty = false;

    BeforeMirroring.push_back(canonizer);
}


void TCombinedHostCanonizer::AddAfterMirroringCanonizer(const THostCanonizerPtr& canonizer) {
    if (IsSmartAdding) {
        ythrow yexception() << "Implicit and explicit ways to add canonizer to TCombinedHostCanonizer are mixed";
    }
    if (canonizer->IsResolvingMirrors()) {
        ythrow yexception() << "Canonizer in stage 'after mirroring' must not resolve mirrors";
    }
    IsEmpty = false;

    AfterMirroring.push_back(canonizer);
}

void TCombinedHostCanonizer::AddMirroringCanonizer(const THostCanonizerPtr& canonizer) {
    if (IsSmartAdding) {
        ythrow yexception() << "Implicit and explicit ways to add canonizer to TCombinedHostCanonizer are mixed";
    }
    if (!canonizer->IsResolvingMirrors()) {
        ythrow yexception() << "Canonizer in stage 'mirroring' must resolve mirrors";
    }
    IsEmpty = false;

    Mirroring.push_back(canonizer);
}

void TCombinedHostCanonizer::AddCanonizer(const THostCanonizerPtr& canonizer) {
    if (!IsSmartAdding && !IsEmpty) {
        ythrow yexception() << "Implicit and explicit ways to add canonizer to TCombinedHostCanonizer are mixed";
    }
    IsEmpty = false;
    IsSmartAdding = true;

    if (CurrentFillingStage == FS_BEFORE_MIRRORING) {
        if (canonizer->IsResolvingMirrors()) {
            CurrentFillingStage = FS_MIRRORING;
        } else {
            BeforeMirroring.push_back(canonizer);
            return;
        }
    }

    if (CurrentFillingStage == FS_MIRRORING) {
        if (!canonizer->IsResolvingMirrors()) {
            CurrentFillingStage = FS_AFTER_MIRRORING;
        } else {
            Mirroring.push_back(canonizer);
            return;
        }
    }

    if (CurrentFillingStage == FS_AFTER_MIRRORING) {
        if (canonizer->IsResolvingMirrors()) {
            ythrow yexception() << "Canonizer in stage 'after mirroring' must not resolve mirrors";
        } else {
            AfterMirroring.push_back(canonizer);
            return;
        }
    }
}

bool TCombinedHostCanonizer::IsResolvingMirrors() const {
    return !Mirroring.empty();
}

TString TCombinedHostCanonizer::CanonizeHost(const TString& host) const {
    TString result = CanonizeBeforeMirrors(host);
    result = CanonizeMirrors(result);
    return CanonizeAfterMirrors(result);
}

TString TCombinedHostCanonizer::CanonizeBeforeMirrors(const TString& host) const {
    TString result(host);
    for (TCanonizers::const_iterator it = BeforeMirroring.begin(); it != BeforeMirroring.end(); ++it) {
        result = (*it)->CanonizeHost(result);
    }
    return result;
}

TString TCombinedHostCanonizer::CanonizeMirrors(const TString& host) const {
    TString result(host);
    for (TCanonizers::const_iterator it = Mirroring.begin(); it != Mirroring.end(); ++it) {
        result = (*it)->CanonizeHost(result);
    }
    return result;
}

TString TCombinedHostCanonizer::CanonizeAfterMirrors(const TString& host) const {
    TString result(host);
    for (TCanonizers::const_iterator it = AfterMirroring.begin(); it != AfterMirroring.end(); ++it) {
        result = (*it)->CanonizeHost(result);
    }
    return result;
}

TMirrorHostCanonizer::TMirrorHostCanonizer(const TString& mirrorsFilename)
    : MirrorResolver(mirrorsFilename)
{
}

bool TMirrorHostCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TMirrorHostCanonizer::CanonizeHost(const TString& host) const {
    return MirrorResolver.GetMainMirror(host);
}

TMirrorMappedHostCanonizer::TMirrorMappedHostCanonizer(const TString& mirrorsFilename)
    : MirrorResolver(mirrorsFilename)
{
}

bool TMirrorMappedHostCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TMirrorMappedHostCanonizer::CanonizeHost(const TString& host) const {
    return MirrorResolver.GetMainMirror(host);
}

TExtMirrorHostCanonizer::TExtMirrorHostCanonizer(const TMirrorResolver& mirrorResolver)
    : MirrorResolver(mirrorResolver)
{
}

bool TExtMirrorHostCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TExtMirrorHostCanonizer::CanonizeHost(const TString& host) const {
    return MirrorResolver.GetMainMirror(host);
}

TOwnerHostCanonizer::TOwnerHostCanonizer(const TString& ownersFilename)
    : OwnersFileName(ownersFilename)
    , OwnerResolver(OwnersFileName)
{
}

TString TOwnerHostCanonizer::CanonizeHost(const TString& host) const {
    return OwnerResolver.GetOwner(host);
}

TString TOwnerHostCanonizer::GetOwnersFileName() const
{
    return OwnersFileName;
}

TString TTrivialHostCanonizer::CanonizeHost(const TString& host) const {
    return host;
}

bool TFakeMirrorHostCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TPunycodeHostCanonizer::CanonizeHost(const TString& host) const {
    if (IsStringASCII(host.begin(), host.end())) {
        return host;
    } else {
        const size_t bufferSize = host.size() + 1;
        TVector<ui32> utf32(bufferSize);

        // url to unicode and lowercase
        {
            TVector<wchar32> wchar32buf(bufferSize);
            size_t numRead, numWritten = 0;

            RECODE_RESULT recodeResult = RecodeToUnicode(CODES_UTF8, host.data(), wchar32buf.data(), host.size(), bufferSize, numRead, numWritten);

            if (recodeResult != RECODE_OK) {
                recodeResult = RecodeToUnicode(CODES_WIN, host.data(), wchar32buf.data(), host.size(), bufferSize, numRead, numWritten);
            }

            if (recodeResult != RECODE_OK) {
                // failed to recode to unicode
                return TString();
            }

            utf32[numWritten] = '\0';
            for (size_t i = 0; i < numWritten; ++i) {
                utf32[i] = ToLower(wchar32buf[i]);
            }
        }

        // unicode to punycode
        {
            char* punycoded;
            const int res = idna_to_ascii_4z(&utf32[0], &punycoded, 0);
            THolder<char, TFree> punycodedHolder(punycoded);

            if (res != IDNA_SUCCESS) {
                return TString();
            }

            return TString(punycoded);
        }
    }
}

TString TLowerCaseHostCanonizer::CanonizeHost(const TString& host) const {
    TString result = host;
    result.to_lower();
    return result;
}

TLastCachedHostCanonizer::TLastCachedHostCanonizer(const IHostCanonizer* canonizer)
    : Lock(0)
    , Canonizer(canonizer)
{
}

bool TLastCachedHostCanonizer::IsResolvingMirrors() const {
    return Canonizer->IsResolvingMirrors();
}

TString TLastCachedHostCanonizer::CanonizeHost(const TString& host) const {
    TString ret;
    // Let's try to use cached result if it possible.

    TPoliteAtomicGuard guard(Lock);
    if (guard.WasAcquired()) {
        // Trying previous result.
        if (host != PrevHost) {
            PrevHost = host;
            PrevResult = Canonizer->CanonizeHost(host);
        }
        ret = PrevResult;
    } else {
        // Previous result is now busy. So, let's work on our own.
        ret = Canonizer->CanonizeHost(host);
    }
    return ret;
}

TString TStripWWWHostCanonizer::CanonizeHost(const TString& host) const {
    return TString{CutWWWPrefix(host)};
}

THostCanonizerPtr GetHostCanonizer(const TString& hostCanonizerParameters) {
    TVector<TString> words;
    TSplitDelimiters delims("@");
    Split(TDelimitersStrictSplit(hostCanonizerParameters, delims), &words);

    if (words.size() % 2 != 0) {
        ythrow yexception() << "Wrong host canonization parameters: " <<  hostCanonizerParameters.data();
    }

    TVector<THostCanonizerPtr> canonizers;

    const size_t canonizerCount = words.size() >> 1;
    for (size_t i = 0; i < canonizerCount; ++i) {
        const TString& type = words[i << 1];
        const TString& parameter = words[(i << 1) | 1];

        THostCanonizerPtr canonizer;
        if (type == TString("o")) {
            canonizer = THostCanonizerPtr(new TOwnerHostCanonizer(parameter));
        } else if (type == TString("m")) {
            canonizer = THostCanonizerPtr(new TMirrorHostCanonizer(parameter));
        } else {
            ythrow yexception() << "Wrong host canonizer type: " <<  type.data();
        }

        canonizers.push_back(canonizer);
    }

    if (canonizerCount == 0) {
        return THostCanonizerPtr(new TTrivialHostCanonizer());
    } else if (canonizerCount == 1) {
        return canonizers[0];
    } else {
        THolder<TCombinedHostCanonizer> combinedCanonizer(new TCombinedHostCanonizer);
        for (TVector<THostCanonizerPtr>::const_iterator it = canonizers.begin(); it != canonizers.end(); ++it) {
            combinedCanonizer->AddCanonizer(*it);
        }
        return THostCanonizerPtr(combinedCanonizer.Release());
    }
}

TMirrorsMappedTrieCanonizer::TMirrorsMappedTrieCanonizer(const TString& filename, EPrechargeMode mode)
    : MirrorTrie(filename.data(), mode)
{
}

bool TMirrorsMappedTrieCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TMirrorsMappedTrieCanonizer::CanonizeHost(const TString& host) const {
    TString res;
    if (MirrorTrie.IsSoft(host.data()))
        return host;
    MirrorTrie.Check(host.data(), res);
    if (res.empty())
        return host;
    return res;
}

TLocalMirrorsCanonizer::TLocalMirrorsCanonizer(const TString& localMirrorsFileName)
{
    TMappedFileInput in(localMirrorsFileName);
    TString line;

    while (in.ReadLine(line)) {
        TVector<TString> columns;
        StringSplitter(line).SplitBySet(" \t").SkipEmpty().Collect(&columns);
        if (columns.size() != 2) {
            continue;
        }
        Mirrors[columns[0]] = columns[1];
    }
}

bool TLocalMirrorsCanonizer::IsResolvingMirrors() const {
    return true;
}

TString TLocalMirrorsCanonizer::CanonizeHost(const TString& host) const {
    const auto& mainMirrorIt = Mirrors.find(host);
    if (mainMirrorIt != Mirrors.end()) {
        return mainMirrorIt->second;
    } else {
        return host;
    }
}

TString TReverseDomainCanonizer::CanonizeHost(const TString& url) const {
    TString host = TString{GetHost(url)};

    // first prize in 'The Most Idiotic InvertDomain Implementation' competition
    if (!host.empty()) {
        TVector<TString> domains;
        StringSplitter(host).Split('.').AddTo(&domains);

        TString result;
        result.reserve(url.size());

        size_t schemeSize = GetHttpPrefixSize(url);

        result += url.substr(0, schemeSize);

        for (int i = domains.ysize()-1; i>0; --i) {
            result += domains[i];
            result.append('.');
        }
        result += domains[0];

        result += url.substr(schemeSize + host.size());

        return result;
    } else {
        return url;
    }
}

bool IsGoodHost(const TString& host) {
    return NTld::InTld(host.data());
}

TString GetCanonicalUrl(const TString& sourceUrl)
{
    TStringBuf normalizationOut = CutHttpPrefix(sourceUrl);
    try {
        TString normalizedUrl = NormalizeUrl(normalizationOut);
        return AddSchemePrefix(normalizedUrl, "http"); // function "AddSchemePrefix" already check prefix
    } catch (yexception&) {
        ythrow TWrongUrlException() << "Wrong URL \"" << sourceUrl << "\" encountered";
    }
}

TString GetCanonicalUrl(const IHostCanonizer* canonizer, const TString& sourceUrl) {
    Y_ASSERT(sourceUrl != nullptr);
    THttpURL parsedNormalizedUrl;
    if (THttpURL::ParsedOK != parsedNormalizedUrl.ParseUri(GetCanonicalUrl(sourceUrl), THttpURL::FeaturesDefault, FULLURL_MAX))
        throw TWrongUrlException() << "Wrong URL \"" << sourceUrl << "\" encountered";

    if (parsedNormalizedUrl.IsNull(THttpURL::FlagHost)) {
        ythrow TWrongUrlException() << "Can not parse hostname from URL \"" <<  sourceUrl << "\"";
    }

    TString canonized = canonizer->CanonizeHost(parsedNormalizedUrl.PrintS(THttpURL::FlagHostPort));
    size_t prefixSize = GetHttpPrefixSize(canonized);
    // to avoid http://https:// cases
    // put scheme and host separately
    if (prefixSize != 0) {
        parsedNormalizedUrl.Set(THttpURL::FieldScheme, canonized.substr(0, prefixSize - 3));
        parsedNormalizedUrl.Set(THttpURL::FieldHost, canonized.substr(prefixSize));
    } else {
        parsedNormalizedUrl.Set(THttpURL::FieldHost, canonized);
    }
    return parsedNormalizedUrl.PrintS();
}

TString GetCanonicalUrlRight(const TString& sourceUrl)
{
    TStringBuf normalizationOut = CutHttpPrefix(sourceUrl);
    if (normalizationOut != sourceUrl)
        return NormalizeUrl(sourceUrl);
    try {
        TString normalizedUrl = NormalizeUrl(normalizationOut);
        return AddSchemePrefix(normalizedUrl, "http"); // function "AddSchemePrefix" already check prefix
    } catch (yexception&) {
        ythrow TWrongUrlException() << "Wrong URL \"" << sourceUrl << "\" encountered";
    }
}

TString GetCanonicalUrlRight(const IHostCanonizer* canonizer, const TString& sourceUrl) {
    Y_ASSERT(sourceUrl != nullptr);
    THttpURL parsedNormalizedUrl;
    TString canonicalUrl = GetCanonicalUrlRight(sourceUrl);
    auto parseResult = parsedNormalizedUrl.ParseUri(canonicalUrl, THttpURL::FeaturesRobot, FULLURL_MAX);
    if (THttpURL::ParsedOK != parseResult)
        throw TWrongUrlException() << "Wrong URL \"" << sourceUrl << "\" encountered" << " canonical url " << canonicalUrl << " parse result " << parseResult;

    if (parsedNormalizedUrl.IsNull(THttpURL::FlagHost)) {
        ythrow TWrongUrlException() << "Can not parse hostname from URL \"" <<  sourceUrl << "\"";
    }

    TString canonized = canonizer->CanonizeHost(parsedNormalizedUrl.PrintS(THttpURL::FlagHostPort | THttpURL::FlagScheme));
    size_t prefixSize = GetHttpPrefixSize(canonized);
    // to avoid http://https:// cases
    // put scheme and host separately
    if (prefixSize != 0) {
        parsedNormalizedUrl.Set(THttpURL::FieldScheme, canonized.substr(0, prefixSize - 3));
        parsedNormalizedUrl.Set(THttpURL::FieldHost, canonized.substr(prefixSize));
    } else {
        parsedNormalizedUrl.Set(THttpURL::FieldHost, canonized);
        parsedNormalizedUrl.Set(THttpURL::FieldScheme, "http");
    }
    return parsedNormalizedUrl.PrintS();
}
