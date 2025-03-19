#include "markers.h"
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>

static const float UNKNOWN_WEIGHT   = 0.0f;
static const size_t BUFFER_SIZE     = 512;

namespace NSnippets {

extern const TString SNIP_MARKERS_NAME = "SnipMarkers";
static const TString SNIP_MARKERS = SNIP_MARKERS_NAME + "=";

using TWeightVect = TVector<float>;
static TWeightVect FillWeights() {
    TWeightVect w(MRK_COUNT, UNKNOWN_WEIGHT);

    w[MRK_YOUTUBE_CHANNEL]              = 0.4886731392f;
    w[MRK_TABLESNIP]                    = 0.5f;
    w[MRK_TABLESNIP_CTR]                = 0.5f;
    w[MRK_CREATIVE_WORK]                = 0.4573991031f;
    w[MRK_ENCYC]                        = 0.5784061697f;
    w[MRK_KINOPOISK]                    = 0.6115107914f;
    w[MRK_LITERA]                       = 0.4858299595f;
    w[MRK_MEDIAWIKI]                    = 0.5571428571f;
    w[MRK_PRODUCT_OFFER]                = 0.5102974828f;
    w[MRK_QUESTION]                     = 0.4940617577f;
    w[MRK_SCHEMA_MOVIE]                 = 0.5460750853f;
    w[MRK_STATIC_ANNOTATION]            = 0.5106888361f;
    w[MRK_TRASH_ANNOTATION]             = 0.5721649485f;
    w[MRK_WEIGHTED_YACA]                = 0.5661103979f;
    w[MRK_WEIGHTED_METADESCR_AND_DMOZ]  = 0.528650647f;
    w[MRK_NEWS]                         = 0.5088161209f;
    w[MRK_FORUM]                        = 0.5669144981f;
    w[MRK_LIST]                         = 0.5160493827f;
    w[MRK_SCHEMA_VTHUMB]                = 0.5f;
    w[MRK_FOTO_RECIPE]                  = 0.8050595238f;
    w[MRK_DIC_ACADEMIC]                 = 0.5f;
    w[MRK_DMOZ]                         = 0.5f;
    w[MRK_EKSISOZLUK_SNIP]              = 0.5f;
    w[MRK_EXTENDED]                     = 0.5f;
    w[MRK_FAKE_REDIRECT]                = 0.5f;
    w[MRK_MOBILE_EXTRA]                 = 0.5f;
    w[MRK_HILITED_URL]                  = 0.5f;
    w[MRK_KINO]                         = 0.5f;
    w[MRK_LASTFM]                       = 0.5f;
    w[MRK_MARKET]                       = 0.6f;
    w[MRK_MOIKRUG]                      = 0.5f;
    w[MRK_MYSPACE]                      = 0.5f;
    w[MRK_OGTEXT]                       = 0.5f;
    w[MRK_PRESSPORT]                    = 0.5f;
    w[MRK_PREVIEW]                      = 0.5f;
    w[MRK_RABOTA]                       = 0.5f;
    w[MRK_RECIPE]                       = 0.5f;
    w[MRK_REFERAT]                      = 0.5f;
    w[MRK_REVIEW]                       = 0.5f;
    w[MRK_ROBOTS_TXT]                   = 0.5f;
    w[MRK_SAHIBINDEN]                   = 0.5f;
    w[MRK_SAHIBINDEN_FAKE]              = 0.5f;
    w[MRK_META_DESCR]                   = 0.5f;
    w[MRK_SITELINKS_BASE]               = 0.5f;
    w[MRK_SOFTWARE]                     = 0.5f;
    w[MRK_SUPPRESS_CYRILLIC]            = 0.5f;
    w[MRK_TEXT_FOR_NPS]                 = 0.5f;
    w[MRK_TITLE_TO_SNIP]                = 0.5f;
    w[MRK_TORRENT_FILM]                 = 0.5490196078f;
    w[MRK_TWITTER]                      = 0.5f;
    w[MRK_URL_MENU]                     = 0.5f;
    w[MRK_VIDEO]                        = 0.8036117381f;
    w[MRK_YACA]                         = 0.5f;
    w[MRK_YOUTUBE]                      = 0.5f;
    w[MRK_VIDEO_DESCR]                  = 0.5f;
    w[MRK_YOUTUBE_CHANNEL_IMG]          = 2.0f;
    return w;
}

static const TWeightVect MARKER_WEIGHTS = FillWeights();

float GetMarkerWeight(EMarker marker) {
    Y_ASSERT(marker < MRK_COUNT);
    return (marker < MRK_COUNT) ? MARKER_WEIGHTS[marker] : UNKNOWN_WEIGHT;
}

TString TMarkersMask::Dump() const {
    TStringStream res;
    TZLibCompress zip(&res, ZLib::ZLib, 6, BUFFER_SIZE);
    Save(&zip);
    zip.Finish();
    return SNIP_MARKERS + Base64Encode(res.Str());
}

bool TMarkersMask::Parse(const TStringBuf& text) {
    size_t beg = text.find(SNIP_MARKERS);
    if (beg == TString::npos)
        return false;

    return Load(text.substr(SNIP_MARKERS.size() + beg));
}

bool TMarkersMask::Load(const TStringBuf& b64Data) {
    try {
        TString data = Base64Decode(b64Data);
        TStringInput rawStream(data);
        TZDecompress unzip(&rawStream, ZLib::ZLib, BUFFER_SIZE);
        TDynBitMap::Load(&unzip);
    } catch (...) {
        return false;
    }
    return true;
}

TString DebugString(const TMarkersMask& markers) {
    TStringStream ss;
    for (EMarker m = static_cast<EMarker>(0); m != MRK_COUNT; m = static_cast<EMarker>(1 + m)) {
        ss << m << ": " << markers.HasMarker(m) << Endl;
    }
    return ss.Str();
}

TString PrintEnabled(const TMarkersMask& markers) {
    TString res;
    for (size_t m = markers.FirstNonZeroBit(); m != markers.Size(); m = markers.NextNonZeroBit(m)) {
        if (res)
            res.append(", ");
        res += ToString(static_cast<EMarker>(m));
    }
    return res;
}

bool ReplaceMarkers(TString& text, const TMarkersMask& markers) {
    size_t pos = text.find(SNIP_MARKERS);
    if (pos != TString::npos) {
        TMarkersMask mm;
        mm.Parse(text);
        text = text.substr(0, pos) + markers.Dump() + text.substr(pos + mm.Dump().length());
        return true;
    }
    return false;
}

double WinProbability(const TVector<double>& winProbs) {
    size_t total = winProbs.size();
    TVector<TVector<double>> mutualProbs(total + 1, TVector<double>(total + 1, 0.));
    // mutualProbs[i][j] stands for probability of exactly j wins amongst first i markers
    mutualProbs[0][0] = 1.0;
    for (size_t i = 1; i <= total; ++i)
        for (size_t j = 0; j <= i; ++j) {
            mutualProbs[i][j] += (1.0 - winProbs[i - 1]) * mutualProbs[i - 1][j];
            if (j > 0) {
                mutualProbs[i][j] += winProbs[i - 1] * mutualProbs[i - 1][j - 1];
            }
        }
    double winProbability = 0.;
    for (size_t j = total / 2 + 1 ; j <= total; ++j) {
        winProbability += mutualProbs[total][j];
    }
    if (total % 2 == 0) {
        winProbability += 0.5 * mutualProbs[total][total / 2];
    }
    return winProbability;
}

double MarkersKPI(const TMarkersMask& markers) {
    // details: https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/Annotation/Projects/MarkersKPI
    TVector<double> winProbs;
    for (size_t m = markers.FirstNonZeroBit(); m != markers.Size(); m = markers.NextNonZeroBit(m))
        winProbs.push_back(GetMarkerWeight(static_cast<EMarker>(m)));
    return WinProbability(winProbs);
}

}; // end of namespace
