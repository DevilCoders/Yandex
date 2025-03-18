#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/stream/format.h>

#include <nginx/modules/strm_packager/src/common/hmac.h>
#include <nginx/modules/strm_packager/src/content/music_uri.h>

namespace NStrm::NPackager {
    TMusicUri::TMusicUri(TStringBuf uri) {
        TVector<TStringBuf> parts = StringSplitter(uri).Split('/').SkipEmpty();

        Y_ENSURE(parts.size() == 9);
        Y_ENSURE(parts[0] == "music");
        Y_ENSURE(parts[3] == "kaltura");

        Bucket = parts[1];           // music-strm-jsons
        TrackId = parts[2];          // 848438.1f597602
                                     // kaltura
        MetaInfo = parts[4];         // 65545786.18.g128
        ValidUntil = parts[5];       // 1613834790
        KeyIv = parts[6];            // 1613797520
        Signature = Unhex(parts[7]); // 157bba2c40b12834

        Y_ENSURE(parts[8] == "caf" || parts[8] == "mp4");
        if (parts[8] == "caf") {
            Container = Caf;
        } else {
            Container = Mp4;
        }
    }

    const TString TMusicUri::GetDescriptionUri() const {
        auto descUri = TStringBuilder()
                       << Join("/", "", Bucket, TrackId, "kaltura", MetaInfo) << ".json";
        return descUri;
    }

    bool TMusicUri::IsValid(TRequestWorker& worker, const TString& secret) const {
        auto signText = TStringBuilder() << TrackId << MetaInfo << ValidUntil << KeyIv;
        auto calculatedHmac = NHmac::Hmac(NHmac::Sha256, secret, signText);
        if (calculatedHmac != Signature) {
            worker.LogDebug() << "The hmacs do not match";
            worker.LogDebug() << "calculated hmac = " << HexText(TStringBuf(calculatedHmac));
            worker.LogDebug() << "incoming   hmac = " << HexText(TStringBuf(Signature));
            return false;
        }

        return true;
    }

    ui8 TMusicUri::Unhex(ui8 c) {
        if ('0' <= c && c <= '9')
            return c - '0';
        else if ('a' <= c && c <= 'f')
            return c - 'a' + 10;
        else if ('A' <= c && c <= 'F')
            return c - 'A' + 10;
        return 0;
    }

    const TStringBuf& TMusicUri::GetKeyIv() const {
        return KeyIv;
    }

    TString TMusicUri::Unhex(const TStringBuf& in) {
        TString result;
        result.reserve(in.size() / 2);

        for (auto p = in.begin(); p != in.end(); ++p) {
            ui8 c = Unhex(*p);
            ++p;
            if (p == in.end())
                break;
            c = (c << 4) + Unhex(*p);
            result.append(c);
        }

        return result;
    }

    TMusicUri::EMusicContainer TMusicUri::GetContainer() const {
        return Container;
    }
}
