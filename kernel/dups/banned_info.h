#pragma once

#include <kernel/dups/banned_info/protos/banned_info.pb.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/stream/input.h>
#include <util/system/types.h>
#include <util/memory/pool.h>

namespace NDups {
    class IBannedInfo {
    public:
        enum class EOriginality {
            Unknown,
            FirstIsOriginal,
            SecondIsOriginal
        };
    public:
        virtual void GetBannedWithKeys(const TStringBuf& url, TVector<ui32>& bannedKeys, TVector<ui32>& bannedHostKeys) const = 0;
        virtual void GetUrlKeys(const TStringBuf& url, TVector<ui32>& keys, ui32& hostKey) const = 0;
        virtual size_t Size() const = 0;
        virtual EOriginality CheckOriginality(TStringBuf firstUrl, TStringBuf secondUrl) const = 0;
        virtual ~IBannedInfo() = default;
    };

    THolder<IBannedInfo> LoadBannedInfoFromTsvFile(const TString& bannedInfoFileName, bool makeOrderInfo = false);
    THolder<IBannedInfo> LoadBannedInfoFromTsvFileString(const TString& bannedInfoFileName, const TStringBuf& bannedInfoFileContent, bool makeOrderInfo = false);
    THolder<IBannedInfo> LoadBannedInfoFromProtoMessage(const TString& bannedInfoFileName, const NDups::NBannedInfo::TBannedInfoListContent& bannedInfoProtoMessage, bool makeOrderInfo = false);

    namespace NBannedInfo {
        ui32 GetHashKeyForUrl(const TStringBuf dataUrl, ui32* hostKey = nullptr);
        ui32 GetHaskKeyForRegEx(const TStringBuf expression);
    } // namespace NBannedInfo

} // namespace NDups
