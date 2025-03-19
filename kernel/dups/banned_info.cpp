#include "banned_info.h"
#include "banned_info_reader.h"
#include "check.h"

#include <kernel/dups/banned_info/dense_vector_pool.h>
#include <kernel/searchlog/errorlog.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/digest/fnv.h>
#include <util/stream/output.h>
#include <util/stream/format.h>
#include <util/stream/file.h>

using namespace NDups;

namespace {

    struct TWarningGuard {
        const TString BannedInfoFileName;
        size_t invalidLines = 0;

        TWarningGuard(const TString& bannedInfoFileName)
            : BannedInfoFileName(bannedInfoFileName)
        {
        }

        ~TWarningGuard() {
            if (invalidLines > 0) {
                SEARCH_WARNING << "[AntiDup] manual dups: " << invalidLines << " invalid lines ignored from " << BannedInfoFileName;
            }
        }

        TWarningGuard& operator++() {
            invalidLines++;
            return *this;
        }
    };

    class TBannedInfo final: public IBannedInfo, public TNonCopyable {
    public:
        TBannedInfo(const TString& bannedInfoFileName, bool makeOrderInfo);
        TBannedInfo(const TString& bannedInfoFileName, const TStringBuf& bannedInfoFileContent, bool makeOrderInfo);
        TBannedInfo(const TString& bannedInfoFileName, const NBannedInfo::TBannedInfoListContent& bannedInfoProtoMessage, bool makeOrderInfo);

        void GetBannedWithKeys(const TStringBuf& url, TVector<ui32>& bannedKeys, TVector<ui32>& bannedHostKeys) const override;
        void GetUrlKeys(const TStringBuf& url, TVector<ui32>& keys, ui32& hostKey) const override;
        size_t Size() const override {
            return Banned.size();
        }
        EOriginality CheckOriginality(TStringBuf firstUrl, TStringBuf secondUrl) const override;

    private: // builder methods
        void LoadFromTsvStream(const TString& bannedInfoFileName, IInputStream& stream);
        void LoadFromProtoMessage(const TString& bannedInfoFileName, const NBannedInfo::TBannedInfoListContent& bannedInfoProtoMessage);
        bool AddExactUrlGroup(TVector<ui32>&& hashes, ui32 groupIdx, bool isHost);
        bool AddRegExGroup(const TString& regex, const ui32 hash);

    private:
        explicit TBannedInfo(bool makeOrderInfo);
        void AddRegexKeys(const TStringBuf& url, TVector<ui32>& keys) const;

    private:
        struct TRegexBan {
            explicit TRegexBan(const TString& regex);
            TRegexBan(const TString& regex, const ui32 hash);
            THolder<RE2> Regex;
            ui32 RegexHash = 0;
        };

        using THashVectorRef = TArrayRef<ui32>;
        using TBannedMap = THashMap<ui32, THashVectorRef, THash<ui32>, TEqualTo<ui32>, TPoolAllocator>;

        TMemoryPool Pool;
        TAppendOnlyDenseVectorPool DenseVectorPool;
        TBannedMap Banned, BannedHosts;
        TVector<TRegexBan> BannedRegex;
        bool MakeOrderInfo = false;
        TBannedMap GroupOrder; // GroupOrder[hash(url)] = {idOfGroup1, orderInGroup1, idOfGroup2, orderInGroup2, ...}, ordered by idOfGroup
    };
}

TBannedInfo::TRegexBan::TRegexBan(const TString& regex)
    : TRegexBan(regex, ::NDups::NBannedInfo::GetHaskKeyForRegEx(regex))
{
}

TBannedInfo::TRegexBan::TRegexBan(const TString& regex, const ui32 hash)
    : Regex(new RE2(regex.data()))
    , RegexHash(hash)
{
}

void TBannedInfo::LoadFromTsvStream(const TString& bannedInfoFileName, IInputStream& stream) {
    auto import = NDups::NBannedInfo::ImportFromTSVStream(stream);
    for (const TString& line : import.Log) {
        SEARCH_WARNING << line << "; File " << bannedInfoFileName;
    }
    if (import.Content) {
        LoadFromProtoMessage(bannedInfoFileName, *import.Content);
    }
}

TBannedInfo::TBannedInfo(bool makeOrderInfo)
    : Pool(4096)
    , DenseVectorPool(&Pool)
    , Banned(&Pool)
    , BannedHosts(&Pool)
    , MakeOrderInfo(makeOrderInfo)
    , GroupOrder(&Pool)
{
}

TBannedInfo::TBannedInfo(const TString& bannedInfoFileName, bool makeOrderInfo)
    : TBannedInfo(makeOrderInfo)
{
    TFileInput input{bannedInfoFileName};
    LoadFromTsvStream(bannedInfoFileName, input);
}

TBannedInfo::TBannedInfo(const TString& bannedInfoFileName, const TStringBuf& bannedInfoFileContent, bool makeOrderInfo)
    : TBannedInfo(makeOrderInfo)
{
    TMemoryInput input{bannedInfoFileContent};
    LoadFromTsvStream(bannedInfoFileName, input);
}

void TBannedInfo::LoadFromProtoMessage(const TString& bannedInfoFileName, const NBannedInfo::TBannedInfoListContent& content) {
    TWarningGuard warnings(bannedInfoFileName);
    ui32 groupIdx = 0;

    if (content.ExactUrlGroupSize() == 0 && content.RegExGroupSize() == 0) {
        SEARCH_WARNING << "[AntiDup] manual dups: emptry proto message: " << bannedInfoFileName;
        ++warnings;
    }

    for (const auto& regExGroup : content.GetRegExGroup()) {
        const TString& ex = regExGroup.GetUrlExpression();
        const ui32 h = regExGroup.GetHash();
        if (!AddRegExGroup(ex, h)) {
            ++warnings;
        }
    }

    TVector<ui32> hashKeys;
    for (const auto& exactUrlGroup : content.GetExactUrlGroup()) {
        if (exactUrlGroup.HashSize() < 2) {
            SEARCH_WARNING << "[AntiDup] manual dups: ignoring proto line (need at least 2 hashes)";
            ++warnings;
        }

        hashKeys.clear();
        for (const ui32 h : exactUrlGroup.GetHash()) {
            hashKeys.push_back(h);
        }

        if (!AddExactUrlGroup(std::move(hashKeys), groupIdx++, exactUrlGroup.GetIsHost())) {
            ++warnings;
        }
    }
}

TBannedInfo::TBannedInfo(const TString& bannedInfoFileName, const NBannedInfo::TBannedInfoListContent& content, bool makeOrderInfo)
    : TBannedInfo(makeOrderInfo)
{
    LoadFromProtoMessage(bannedInfoFileName, content);
}

bool TBannedInfo::AddExactUrlGroup(TVector<ui32>&& hashKeys, ui32 groupIdx, bool isHost) {
    NDups::NBannedInfo::RemoveDuplicatesPreservingOrder(hashKeys);
    if (hashKeys.size() < 2) {
        return false;
    }

    auto& target = (isHost ? BannedHosts : Banned);
    const size_t bigGroupSizeThreshold = Max();
    if (hashKeys.size() <= bigGroupSizeThreshold) {
        for (const ui32 h1 : hashKeys) {
            THashVectorRef fill = DenseVectorPool.GrowArray(target[h1], hashKeys.size() - 1);
            auto fillIter = fill.begin();
            for (const ui32 h2 : hashKeys) {
                if (h1 != h2) {
                    *fillIter = h2;
                    ++fillIter;
                }
            }
            Y_VERIFY_DEBUG(fillIter == fill.end());
        }
    } else {
        const ui32 groupHash = Accumulate(hashKeys, 0xBA44ED, CombineHashes<ui32>);
        for (const ui32 h : hashKeys) {
            THashVectorRef fill = DenseVectorPool.GrowArray(target[h], 1);
            Y_VERIFY_DEBUG(fill.size() == 1);
            fill[0] = groupHash;
        }
    }
    if (MakeOrderInfo) {
        for (ui32 pos = 0; pos < hashKeys.size(); pos++) {
            const ui32 h = hashKeys[pos];
            THashVectorRef fill = DenseVectorPool.GrowArray(GroupOrder[h], 2);
            auto fillIter = fill.begin();
            *fillIter++ = groupIdx;
            *fillIter = pos;
        }
    }
    return true;
}

bool TBannedInfo::AddRegExGroup(const TString& regex, const ui32 hash) {
    if (regex.empty()) {
        return false;
    }
    if (MakeOrderInfo) {
        SEARCH_WARNING << "[AntiDup] manual ordered dups: regex group has no defined ordering";
    }
    BannedRegex.emplace_back(regex, hash);
    return true;
}

void TBannedInfo::GetBannedWithKeys(const TStringBuf& url, TVector<ui32>& bannedKeys, TVector<ui32>& bannedHostKeys) const {
    ui32 hostKey = 0;
    const ui32 key = ::NDups::NBannedInfo::GetHashKeyForUrl(url, &hostKey);
    if (const auto b = Banned.FindPtr(key)) {
        bannedKeys.insert(bannedKeys.end(), b->begin(), b->end());
    }
    if (const auto b = BannedHosts.FindPtr(hostKey)) {
        bannedHostKeys.insert(bannedHostKeys.end(), b->begin(), b->end());
    }
    AddRegexKeys(url, bannedKeys);
}

void TBannedInfo::GetUrlKeys(const TStringBuf& url, TVector<ui32>& keys, ui32& hostKey) const {
    keys.push_back(::NDups::NBannedInfo::GetHashKeyForUrl(url, &hostKey));
    AddRegexKeys(url, keys);
}

void TBannedInfo::AddRegexKeys(const TStringBuf& url, TVector<ui32>& keys) const {
    for (const auto& regexBan : BannedRegex) {
        const re2::StringPiece input(url.data(), url.size());
        if (regexBan.Regex && RE2::FullMatch(input, *regexBan.Regex)) {
            keys.push_back(regexBan.RegexHash);
        }
    }
}

TBannedInfo::EOriginality TBannedInfo::CheckOriginality(TStringBuf firstUrl, TStringBuf secondUrl) const {
    Y_ASSERT(MakeOrderInfo);
    const ui32 key1 = ::NDups::NBannedInfo::GetHashKeyForUrl(firstUrl);
    const ui32 key2 = ::NDups::NBannedInfo::GetHashKeyForUrl(secondUrl);
    const TArrayRef<ui32>* groups1 = GroupOrder.FindPtr(key1);
    const TArrayRef<ui32>* groups2 = GroupOrder.FindPtr(key2);
    if (!groups1 || !groups2)
        return TBannedInfo::EOriginality::Unknown;
    Y_ASSERT(groups1->size() % 2 == 0 && groups2->size() % 2 == 0);
    TArrayRef<ui32>::const_iterator it1 = groups1->begin(), it2 = groups2->begin();
    while (it1 != groups1->end() && it2 != groups2->end()) {
        if (*it1 == *it2) {
            ++it1;
            ++it2;
            return *it1 <= *it2 ? TBannedInfo::EOriginality::FirstIsOriginal : TBannedInfo::EOriginality::SecondIsOriginal;
        }
        if (*it1 < *it2) {
            ++it1;
            ++it1;
        } else {
            ++it2;
            ++it2;
        }
    }
    return TBannedInfo::EOriginality::Unknown;
}

THolder<NDups::IBannedInfo> NDups::LoadBannedInfoFromTsvFile(const TString& bannedInfoFileName, bool makeOrderInfo) {
    return MakeHolder<TBannedInfo>(bannedInfoFileName, makeOrderInfo);
}

THolder<NDups::IBannedInfo> NDups::LoadBannedInfoFromTsvFileString(const TString& bannedInfoFileName, const TStringBuf& bannedInfoFileContent, bool makeOrderInfo) {
    return MakeHolder<TBannedInfo>(bannedInfoFileName, bannedInfoFileContent, makeOrderInfo);
}

THolder<NDups::IBannedInfo> NDups::LoadBannedInfoFromProtoMessage(const TString& bannedInfoFileName, const NDups::NBannedInfo::TBannedInfoListContent& bannedInfoProtoMessage, bool makeOrderInfo) {
    return MakeHolder<TBannedInfo>(bannedInfoFileName, bannedInfoProtoMessage, makeOrderInfo);
}

ui32 NDups::NBannedInfo::GetHashKeyForUrl(const TStringBuf dataUrl, ui32* hostKey) {
    return ::NDups::GetHashKeyForUrl(dataUrl, true, hostKey);
}

ui32 NDups::NBannedInfo::GetHaskKeyForRegEx(const TStringBuf expression) {
    return FnvHash<ui32>(expression);
}
