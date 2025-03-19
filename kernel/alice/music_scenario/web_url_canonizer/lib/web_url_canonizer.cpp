#include "web_url_canonizer.h"

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/string/builder.h>
#include <util/string/split.h>

namespace NAlice {

    namespace NMusic {

        bool IsValidItemId(const TStringBuf id) {
            ui64 val;
            return TryFromString(id, val);
        }


        // Do not change the function without agreement with (olegator and (hommforever or gotmanov))
        TParseMusicUrlResult ParseMusicDataFromDocUrl(const TStringBuf url) {
            const TStringBuf host = GetOnlyHost(url);
            const TVector<TStringBuf> hostParts = StringSplitter(host).Split('.').ToList<TStringBuf>();
            if (hostParts.size() != 3 || hostParts[0] != "music" || hostParts[1] != "yandex") {
                return TParseMusicUrlResult();
            }

            TStringBuf path = GetPathAndQuery(url).NextTok('?');

            if (!path.SkipPrefix(TStringBuf("/"))) {
                return TParseMusicUrlResult();
            }

            const TStringBuf partType = path.NextTok('/');
            const TStringBuf partId = path.NextTok('/');

            if (partType.empty() || partId.empty()) {
                return TParseMusicUrlResult();
            }

            if (partType != TStringBuf("users") && !IsValidItemId(partId)) {
                return TParseMusicUrlResult();
            }

            if (partType == TStringBuf("album")) {
                const TStringBuf secondPartType = path.NextTok('/');
                const TStringBuf secondPartId = path.NextTok('/');
                if (secondPartId.empty()) {
                    return TParseMusicUrlResult(EMusicUrlType::Album, partType, partId);
                } else {
                    if (!IsValidItemId(secondPartId)) {
                        return TParseMusicUrlResult();
                    }
                    return TParseMusicUrlResult(EMusicUrlType::Track,
                                                partType, partId,
                                                secondPartType, secondPartId);
                }
            } else if (partType == TStringBuf("track")) {
                return TParseMusicUrlResult(EMusicUrlType::Track, partType, partId);
            } else if (partType == TStringBuf("artist")) {
                return TParseMusicUrlResult(EMusicUrlType::Artist, partType, partId);
            } else if (partType == TStringBuf("users")) {
                const TStringBuf secondPartType = path.NextTok('/');
                const TStringBuf secondPartId = path.NextTok('/');
                if (secondPartId.empty()) {
                    return TParseMusicUrlResult();
                }
                return TParseMusicUrlResult(EMusicUrlType::Playlist,
                                            partType, partId,
                                            secondPartType, secondPartId);
            }
            return TParseMusicUrlResult();
        }

        TString CanonizeMusicUrl(const TStringBuf url) {
            const TParseMusicUrlResult parseMusicUrlResult = ParseMusicDataFromDocUrl(url);

            TStringBuilder resultBuilder = TStringBuilder() << "https://music.yandex.ru";

            switch (parseMusicUrlResult.Type) {
                case EMusicUrlType::None:
                    return "";

                case EMusicUrlType::Album:
                    Y_ASSERT(parseMusicUrlResult.Parts.size() == 1);
                    resultBuilder << "/album/" << parseMusicUrlResult.Parts.front().PartValue;
                    break;

                case EMusicUrlType::Track:
                    Y_ASSERT(parseMusicUrlResult.Parts.size() <= 2);
                    resultBuilder << "/track/" << parseMusicUrlResult.Parts.back().PartValue;
                    break;

                case EMusicUrlType::Playlist: {
                    Y_ASSERT(parseMusicUrlResult.Parts.size() == 2);
                    const TStringBuf firstPartId = parseMusicUrlResult.Parts[0].PartValue;
                    const TStringBuf secondPartId = parseMusicUrlResult.Parts[1].PartValue;
                    TString userLogin(firstPartId);
                    Quote(userLogin, "");
                    resultBuilder << "/users/" << userLogin << "/playlists/" << secondPartId << "?rich-tracks=false";
                    break;
                }

                case EMusicUrlType::Artist:
                    Y_ASSERT(parseMusicUrlResult.Parts.size() == 1);
                    resultBuilder << "/artist/" << parseMusicUrlResult.Parts[0].PartValue;
                    break;
            }

            return resultBuilder;
        }

    } // namespace NMusic

} // namespace NAlice
