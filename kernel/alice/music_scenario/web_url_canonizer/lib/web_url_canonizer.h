#pragma once

#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NAlice {

    namespace NMusic {

        enum class EMusicUrlType {
            None = 0,
            Album = 1,
            Track = 2,
            Playlist = 3,
            Artist = 4,
        };

        struct TParseMusicUrlResultPart {
            const TString PartName;
            const TString PartValue;

            TParseMusicUrlResultPart(const TStringBuf partName, const TStringBuf partValue)
                : PartName(partName)
                , PartValue(partValue)
            {
            }
        };

        struct TParseMusicUrlResult {
            const EMusicUrlType Type = EMusicUrlType::None;
            const TVector<TParseMusicUrlResultPart> Parts;

            TParseMusicUrlResult()
            {
            }

            TParseMusicUrlResult(const EMusicUrlType type, const TStringBuf partName, const TStringBuf partValue)
                : Type(type)
                , Parts({ TParseMusicUrlResultPart(partName, partValue) })
            {
                Y_ASSERT(Type == EMusicUrlType::Album ||
                         Type == EMusicUrlType::Track ||
                         Type == EMusicUrlType::Artist);
            }

            TParseMusicUrlResult(
                const EMusicUrlType type,
                const TStringBuf parentPartName, const TStringBuf parentPartValue,
                const TStringBuf partName, const TStringBuf partValue
            )
                : Type(type)
                , Parts({ TParseMusicUrlResultPart(parentPartName, parentPartValue), TParseMusicUrlResultPart(partName, partValue) })
            {
                Y_ASSERT(Type == EMusicUrlType::Playlist || Type == EMusicUrlType::Track);
            }
        };

        TParseMusicUrlResult ParseMusicDataFromDocUrl(TStringBuf url);

        TString CanonizeMusicUrl(TStringBuf url);

    } // namespace NMusic

} // namespace NAlice
