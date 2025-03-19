#include "url_sequences.h"

#include "urlseq_writer.h" // for ShortenUrl

#include <kernel/yawklib/dupes.h>

#include <library/cpp/charset/recyr.hh>

#include <util/generic/ymath.h>
#include <util/string/split.h>

namespace NSequences {

namespace NDetails {

    template <class RI>
    TUrlCalcerImpl<RI>::TUrlCalcerImpl(
        TReaderImpl* reader,
        TString const& request,
        bool newQuorumQueryEditDistIsEnabled)
        : Request(request)
        , IsEditDistEnabled(newQuorumQueryEditDistIsEnabled)
    {
        Y_ASSERT(reader);
        Reader.Reset(reader);
        TrigramCalcer.Reset(new ::NStringMatchTracker::TQueryTrigramOverDocCalcer(false));
        LcsCalcer.Reset(new ::NStringMatchTracker::TLCSCalcer);
    }

    template <class RI>
    bool TUrlCalcerImpl<RI>::CalcFractionFeatures(ui32 docId, TUrlFractionFeatures& features) const {
        Reader->InitDoc(docId);
        const TString& url = Reader->GetUrl();
        if (url.empty()) {
            return false;
        }

        const bool hasSlash = (Reader->GetDomainLen() + Reader->GetPathLen() + 1u) == url.size();

        if (!TrigramCalcer->IsInited()) {
            TrigramCalcer->NewQuery(Request);
        }

        const size_t domainLen = Reader->GetDomainLen();
        const size_t pathLen = Reader->GetPathLen();

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(url.data(), url.data() + domainLen);
        const size_t domainMatch = TrigramCalcer->GetRawMatch();

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(url.data() + domainLen + hasSlash, url.end());
        const size_t pathAndParamsMatch = TrigramCalcer->GetRawMatch();

        features.DomainFraction = domainLen ? domainMatch / (float)domainLen : 0.0f;
        features.PathAndParamsFraction = pathLen ? pathAndParamsMatch / (float)pathLen : 0.0f;

        const size_t fullUrlMatch = domainMatch + pathAndParamsMatch;
        const size_t fullUrlLen = domainLen + pathLen;
        features.FullUrlFraction = fullUrlLen ? fullUrlMatch / (float)fullUrlLen : 0.0f;

        return true;
    }

    template <class RI>
    bool TUrlCalcerImpl<RI>::CalcSimilarityFeatures(ui32 docId, TUrlSimilarityFeatures& features) const {
        Reader->InitDoc(docId);
        const TString& url = Reader->GetUrl();
        if (url.empty()) {
            return false;
        }

        const size_t domainLen = Reader->GetDomainLen();
        const size_t pathLen = Reader->GetPathLen();
        const size_t fullUrlLen = domainLen + pathLen;

        if (!TrigramCalcer->IsInited()) {
            TrigramCalcer->NewQuery(Request);
        }

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(url.data(), url.data() + domainLen);

        if (!LcsCalcer->IsInited()) {
            LcsCalcer->NewQuery(Request);
        }

        LcsCalcer->Reset();
        LcsCalcer->ProcessDoc(url.begin(), url.end());

        features.DomainSimilarityFixed = TrigramCalcer->NormalizeAsSimilarityFixed().Value;
        const size_t lcs = LcsCalcer->GetRawMatch();
        features.QueryUrlLCSNormalizedByUrl = (fullUrlLen && lcs) ? float(lcs) / fullUrlLen : 0.0f;
        features.QueryUrlLCSNormalizedByQuery = (Request.size() && lcs) ? float(lcs) / Request.size() : 0.0f;
        if (IsEditDistEnabled) {
            int editDist = LevenshteinDist(Request, url);
            features.EditDistNormalizedByMaxLen = (fullUrlLen || Request.size()) ? float(editDist) / Max(fullUrlLen, Request.size()) : 0.0f;
        }
        return true;
    }

    template <class RI>
    bool TUrlCalcerImpl<RI>::CalcUrlTextFeatures(ui32 docId, TUrlTextFeatures& features) const {
        using EMusicUrlType = NAlice::NMusic::EMusicUrlType;

        Reader->InitDoc(docId);
        features.IsHttps = static_cast<float>(Reader->GetHasHttps());
        EMusicUrlType musicUrlType = static_cast<EMusicUrlType>(Reader->GetYandexMusicUrlType());
        features.IsYandexMusicAlbum = static_cast<float>(musicUrlType == EMusicUrlType::Album);
        features.IsYandexMusicTrack = static_cast<float>(musicUrlType == EMusicUrlType::Track);
        return true;
    }

    template <class RI>
    bool TUrlCalcerImpl<RI>::CalcVideoFeatures(ui32 docId, TUrlVideoFeatures& features) const {
        Reader->InitDoc(docId);
        const TString& url = Reader->GetUrl();
        if (url.empty()) {
            return false;
        }

        const size_t domainLen = Reader->GetDomainLen();
        const bool hasSlash = (domainLen + Reader->GetPathLen() + 1u) == url.size();

        if (!TrigramCalcer->IsInited()) {
            TrigramCalcer->NewQuery(Request);
        }

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(url.data(), url.data() + domainLen);
        features.DomainSimilarityFixed = TrigramCalcer->NormalizeAsSimilarityFixed().Value;

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(url.data() + domainLen + hasSlash, url.end());
        features.PathAndParamsFraction = TrigramCalcer->NormalizeAsDocLen().Value;

        if (!LcsCalcer->IsInited()) {
            LcsCalcer->NewQuery(Request);
        }

        LcsCalcer->Reset();
        LcsCalcer->ProcessDoc(url.data(), url.data() + domainLen);
        features.QueryDomainLCSQueryFraction = LcsCalcer->NormalizeAsQueryLen().Value;

        return true;
    }

    template <class RI>
    TTitleCalcerImpl<RI>::TTitleCalcerImpl(TReaderImpl* reader, const TString& request)
        : Request(request)
    {
        Y_ASSERT(reader);
        Reader.Reset(reader);
        TrigramCalcer.Reset(new ::NStringMatchTracker::TQueryTrigramOverDocCalcer(true));
    }

    template <class RI>
    bool TTitleCalcerImpl<RI>::CalcFeatures(ui32 docId, TTitleFeatures& features) const {
        if (!Reader->CheckDoc(docId)) {
            return false;
        }

        Reader->InitDoc(docId);
        const TString& title = Reader->GetUrl();
        if (title.empty()) {
            return false;
        }

        Y_ASSERT(Reader->GetPathLen() == 0);

        if (!TrigramCalcer->IsInited()) {
            TrigramCalcer->NewQuery(Request);
        }

        TrigramCalcer->Reset();
        TrigramCalcer->ProcessDoc(title.begin(), title.end());

        features.TitleTrigramsQ = TrigramCalcer->NormalizeAsDocLen().Value;
        features.TitleTrigramsT = TrigramCalcer->NormalizeAsQueryLen().Value;

        return true;
    }

    template class TUrlCalcerImpl<TReader>;
    template class TUrlCalcerImpl<TRealTimeReader<IOnlineUrlStorage>>;

    template class TTitleCalcerImpl<TReader>;
    template class TTitleCalcerImpl<TRealTimeReader<IOnlineTitleStorage>>;
}

}
