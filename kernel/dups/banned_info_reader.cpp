#include "banned_info_reader.h"
#include "banned_info.h"

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/deque.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <util/digest/numeric.h>
#include <util/string/split.h>

namespace NDups {
    namespace NBannedInfo {
        class TTSVReader {
        public:
            TTSVReader(const TImportParameters params)
                : Params(params)
            {
            }

            void Import(IInputStream& stream) {
                if (!Result.Content) {
                    Result.Content = MakeHolder<TBannedInfoListContent>();
                }

                TString pairLine;
                ui64 warnings = 0;
                Line = 0;

                while (stream.ReadLine(pairLine)) {
                    ++Line;

                    pairLine = StripString(pairLine);
                    if (pairLine.empty()) {
                        continue;
                    }
                    if (pairLine.StartsWith('#')) {
                        continue;
                    }

                    if (!AddNextLine(pairLine)) {
                        ++warnings;
                    }
                }

                Result.WarningsNum = warnings;
                if (warnings) {
                    Report(TStringBuilder() << warnings << "  warnings", false);
                }
            }

        private:
            void Report(const TString& message, bool addLine = true) {
                TStringStream ss;
                ss << "[AntiDup] manual dups";
                if (addLine) {
                    ss << " line " << Line;
                }
                ss << ": " << message;
                Result.Log.push_back(ss.Str());
            }

            bool AddRegExGroup(const TString& regex) {
                if (regex.empty()) {
                    Report("Empty regular expression");
                    return false;
                }
                if (Params.EarlyValidation) {
                    RE2 e{regex};
                    if (!e.ok()) {
                        Report(TStringBuilder() << "Invalid regular expression: " << e.error());
                        return false;
                    }
                }

                const ui32 hash = ::NDups::NBannedInfo::GetHaskKeyForRegEx(regex);

                auto* group = Result.Content->AddRegExGroup();
                group->SetUrlExpression(regex);
                group->SetHash(hash);
                return true;
            }

            bool AddExactUrlGroup(TVector<ui32>& hashKeys, bool isHost) {
                RemoveDuplicatesPreservingOrder(hashKeys);
                if (hashKeys.size() < 2) {
                    ++Result.TrivialGroupsNum;
                    if (Params.SkipTrivialGroups) {
                        return true;
                    } else {
                        Report(TStringBuilder() << "Ignoring line (need at least 2 different hashes but got " << hashKeys.size() << ")");
                        return false;
                    }
                }
                if (Params.TrackContentRepeats) {
                    TContentTrackRecord record{Line};
                    auto ins = ContentTrack.emplace(hashKeys, record);
                    if (!ins.second) {
                        Report(TStringBuilder() << "Duplicate hash set. Previous set defined in line " << ins.first->second.LineNumber);
                        return false;
                    }
                }
                auto* group = Result.Content->AddExactUrlGroup();
                for (const ui32 h : hashKeys) {
                    group->AddHash(h);
                }
                if (isHost) {
                    group->SetIsHost(true);
                }
                return true;
            }

            bool AddExactUrlGroup(TVector<TString>& bannedUrls, const TString& pairLine, bool isHost) {
                if (Params.EarlyValidation) {
                    RemoveDuplicatesPreservingOrder(bannedUrls);
                    if (bannedUrls.size() < 2) {
                        Report(TStringBuilder() << "ignoring line (need at least 2 different urls): " << pairLine);
                        return false;
                    }
                }

                TVector<ui32>& hashKeys = HashKeysWorkingSet;
                hashKeys.clear();
                for (const auto& url : bannedUrls) {
                    ui32 hostKey = 0;
                    ui32 key = ::NDups::NBannedInfo::GetHashKeyForUrl(url, &hostKey);
                    hashKeys.push_back(isHost ? hostKey : key);
                }

                return AddExactUrlGroup(hashKeys, isHost);
            }

            bool AddNextLine(TString& pairLine) {
                TVector<TString>& bannedUrls = UrlsWorkingSet;
                bannedUrls.clear();

                StringSplitter(pairLine).SplitBySet(" \t").SkipEmpty().Collect(&bannedUrls);

                if (bannedUrls.size() < 2) {
                    Report(TStringBuilder() << "ignoring line (need at least 2 tab separated fields): " << pairLine);
                    return false;
                }

                if (bannedUrls.size() == 2 && bannedUrls.front() == "REGEX") {
                    const TString ex = bannedUrls.back();
                    return AddRegExGroup(ex);
                }

                bool isHost = false;
                if (bannedUrls.front() == "HOST") {
                    isHost = true;
                    bannedUrls.erase(bannedUrls.begin());
                }

                return AddExactUrlGroup(bannedUrls, pairLine, isHost);
            }

        public:
            TTSVImportResult Result;

        private:
            struct TContentTrackRecord {
                size_t LineNumber = -1;
            };
            struct THashVectorHash {
                size_t operator()(const TVector<ui32>& hashVec) const {
                    return Accumulate(hashVec, 0xBA44ED, CombineHashes<ui64>);
                }
            };

        private:
            const TImportParameters Params;
            ui64 Line = 0;
            TVector<ui32> HashKeysWorkingSet;
            TVector<TString> UrlsWorkingSet;
            THashMap<TVector<ui32>, TContentTrackRecord, THashVectorHash> ContentTrack;
        };

        TTSVImportResult ImportFromTSVStream(IInputStream& stream, const TImportParameters params) {
            TTSVReader reader{params};
            reader.Import(stream);
            return std::move(reader.Result);
        }
    }
}
