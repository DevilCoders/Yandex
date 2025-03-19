#pragma once

#include <kernel/url/url_canonizer.h>
#include <ysite/yandex/dates/doctime.h>
#include <util/generic/map.h>

#include "canonizers.h"
#include "erfattrs.h"
#include "config.h"

extern const char* isCommByKeyWordsCategory; // = "secta1" - 36000000 catalog value family
extern const char* isCommByKeyWordsValue;    // = "2"
extern const ui32 isCommByKeyWordsGroupValue; // = 36000002 - "secta1" group plus IsCommByKeywordsValue

class TErfs : public TVector<SDocErfInfo3> {
public:
    typedef value_type data_type;
    TErfs()
        : TVector<SDocErfInfo3>()
    {
    }
    TErfs(size_type count)
        : TVector<SDocErfInfo3>(count)
    {
    }
};

class TErfsRemap {
public:
    TErfs Erfs;
    TVector<ui32> Remap;

public:
    typedef SDocErfInfo3 value_type;
    typedef value_type data_type;

public:
    SDocErfInfo3& operator[](int index) {
        return Erfs[Remap[index]];
    }

    void resize(size_t size) {
        Erfs.resize(size);
        Remap.resize(size);
        for (size_t i = 0; i < size; i++)
            Remap[i] = i;
    }

    size_t size() {
        return Erfs.size() ? Remap.back() + 1 : 0;
    }
};

class TErfs2 : public TVector<SDocErf2Info> {
public:
    typedef value_type data_type;
    TErfs2()
        : TVector<SDocErf2Info>()
    {
    }

    TErfs2(size_type count)
        : TVector<SDocErf2Info>(count)
    {
    }
};

class TErfReader {
public:
    THolder<TIFStream> Input;
    ui32 RecordSize;
    ui32 Version;
    ui32 Time;
    ui32 CurDoc;
    TBlob GenericData;

public:
    void Open(const char* name) {
        Input = MakeHolder<TIFStream>(name);
        char head[TArrayWithHeadBase::N_HEAD_SIZE];
        Input->LoadOrFail(head, TArrayWithHeadBase::N_HEAD_SIZE);
        TMemoryInput headerInput(head, TArrayWithHeadBase::N_HEAD_SIZE);
        GetArrayHead(&headerInput, &RecordSize, &Version, &Time, &GenericData);
        CurDoc = 0;
    }
    template<class TInfo>
    bool Next(TInfo& erf, ui32& docId) {
        if (Input->Load(&erf, RecordSize) != RecordSize)
            return false;
        docId = CurDoc++;
        return true;
    }
};


class TRegErfs : public TMap<ui32, TRegionsInfo> {
public:
    typedef mapped_type data_type;
};

class SDocErfInfo3;
class THostErfInfo;
namespace NRealTime {
    class THostFactors;
    class TDocFactors;
    class TDocFactors_TOrangeDocFactors;
    class TIndexedDoc;
    class TSpamFactors;
}
class TAnchorText;

struct TDocInfoEx;

namespace NGroupingAttrs {
    class TAttrWeightPropagator;
}

class TFullDocAttrs;

template <class TErf>
void FillErfFromDaterStats(const NDater::TDaterStats& stats, TErf& erf, time_t now) {
    using namespace NDater;
    ui32 nowYear = NDatetime::TSimpleTM::New(now).RealYear();

    ui32 curYearInTitleContent = stats.CountYears(TDaterDate::FromTitle, nowYear)
                    + stats.CountYears(TDaterDate::FromMainContent, nowYear)
                    + stats.CountYears(TDaterDate::FromMainContentStart, nowYear)
                    + stats.CountYears(TDaterDate::FromMainContentEnd, nowYear)
                    + stats.CountYears(TDaterDate::FromBeforeMainContent, nowYear)
                    + stats.CountYears(TDaterDate::FromAfterMainContent, nowYear)
                    + stats.CountYears(TDaterDate::FromContent, nowYear);

    ui32 prevYearsInTitleContent = stats.CountYears(TDaterDate::FromTitle)
                    + stats.CountYears(TDaterDate::FromMainContent)
                    + stats.CountYears(TDaterDate::FromMainContentStart)
                    + stats.CountYears(TDaterDate::FromMainContentEnd)
                    + stats.CountYears(TDaterDate::FromBeforeMainContent)
                    + stats.CountYears(TDaterDate::FromAfterMainContent)
                    + stats.CountYears(TDaterDate::FromContent)
                    - curYearInTitleContent;

    ui32 fullDatesInText = stats.CountYears(TDaterDate::FromMainContent, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromBeforeMainContent, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromAfterMainContent, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromMainContentStart, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromMainContentEnd, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromContent, TDaterDate::ModeFull)
                    + stats.CountYears(TDaterDate::FromText, TDaterDate::ModeFull);

    ui32 maxYearInContent = Max(stats.MaxYear(TDaterDate::FromBeforeMainContent),
                                Max(stats.MaxYear(TDaterDate::FromAfterMainContent),
                                    Max(stats.MaxYear(TDaterDate::FromMainContentStart),
                                        Max(stats.MaxYear(TDaterDate::FromMainContentEnd),
                                            Max(stats.MaxYear(TDaterDate::FromMainContent),
                                                stats.MaxYear(TDaterDate::FromContent))))));

    ui32 uniqYearsInContent = stats.CountUniqYears(TDaterDate::FromMainContent)
                    + stats.CountUniqYears(TDaterDate::FromBeforeMainContent)
                    + stats.CountUniqYears(TDaterDate::FromAfterMainContent)
                    + stats.CountUniqYears(TDaterDate::FromMainContentStart)
                    + stats.CountUniqYears(TDaterDate::FromMainContentEnd)
                    + stats.CountUniqYears(TDaterDate::FromContent);

    ui32 maxyearintitle = stats.MaxYear(TDaterDate::FromTitle);
    ui32 minyearintitle = stats.MinYear(TDaterDate::FromTitle);
    ui8 yearNormLikelihood = stats.YearNormLikelihood();
    ui8 averageSourceSegment = stats.AverageSourceSegment();

    erf.DaterStatsPrevYearsInTitleContent = Min<ui32>(prevYearsInTitleContent, 31);
    erf.DaterStatsFullDatesInText = Min<ui32>(fullDatesInText, 31);
    erf.DaterStatsMaxYearInContent = Min<ui32>(maxYearInContent - TDaterDate::ErfZeroYear, 31);
    erf.DaterStatsUniqYearsInContent = Min<ui32>(uniqYearsInContent, 7);
    erf.DaterStatsMinYearInTitle = Min<ui32>(minyearintitle - TDaterDate::ErfZeroYear, 31);
    erf.DaterStatsMaxYearInTitle = Min<ui32>(maxyearintitle - TDaterDate::ErfZeroYear, 31);
    erf.DaterStatsYearNormLikelihood = yearNormLikelihood;
    erf.DaterStatsAverageSourceSegment = averageSourceSegment;
}

void FillErfFromErfAttrs(const TErfAttrs& attrs, SDocErfInfo3& erf);

// nNews nCatalog nShop IsLJ IsLib IsObsolete
void FillUrlBasedFactors(const TString& szUrl, SDocErfInfo3& erf);
void FillUrlBasedFactors(const TString& szUrl, const SDocErfInfo3::TFieldMask& fieldMask, SDocErfInfo3& erf);
void FillUrlBasedFactors(const TString& szUrl, NRealTime::TDocFactors& proto);

// Get isUa, iaCom, isRu, UrlHasDigits, UrlLen, IsForum, IsBlog
void FillErfFromDocFactorsProto(const NRealTime::TDocFactors& proto, SDocErfInfo3& erf);
void FillDocFactorsFromUrl(const TString& url0, NRealTime::TDocFactors& proto, const IHostCanonizer* mainPageCanonizer = nullptr);

// AddTime, Language, Hops, IsHTML
void FillOrangeDocFactorsFromDocInfo(const TDocInfoEx& docInfo, NRealTime::TDocFactors_TOrangeDocFactors& proto, size_t currentTime = 0);
// PubTime
void FillOrangeDocFactorsFromAnchorText(const TAnchorText& anchorText, NRealTime::TDocFactors_TOrangeDocFactors& proto);

// Dater(day, month, year)
void FillErfDaterFieldsFromErfAttrs(const TErfAttrs& attrs, SDocErfInfo3& erf);

void ReadDaterStats(NDater::TDaterStats& stats, const TErfAttrs& attrs);
NDater::TTimeAndSource MakeTimeAndSource(const SDocErfInfo3& erf, const NDater::TDaterStats& stats, time_t currtime);
void FillErfFromDaterStats(SDocErfInfo3& erf, const NDater::TDaterStats& stats, time_t now);
// IsBlog, IsForum, PornoWeight
void FillErfFromSpamFactors(const NRealTime::TSpamFactors& spamFactors, SDocErfInfo3* erfInfo);

// IsComm IsSEO, IsPorno, HasPayments
void FillErfQueryFactors(const TErfAttrs& attrs, SDocErfInfo3& erf);
// TitleComm, TitleBM25Ex
void FillErfTitleFeatures(const TErfAttrs& attrs, SDocErfInfo3& erf);
void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, SDocErfInfo3& erfInfo3, time_t currentTime);
void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, const TErfAttrs& erfAttrs, SDocErfInfo3& erfInfo3, time_t currentTime);
void FillErfAttrs(const NRealTime::TIndexedDoc& indexedDoc, const TErfAttrs& erfAttrs, SDocErfInfo3& erfInfo3, time_t currentTime, const TString& url4UrlHash);

// isUkr and IsCommByKeywords
void ProcessDocAttrs(NGroupingAttrs::TAttrWeightPropagator& propagator, const TFullDocAttrs& attrs, NRealTime::TDocFactors& docFactors);

void FillHostFactorsFromHost(const TString& host, const TCanonizers& canonizers, NRealTime::THostFactors& proto);
void FillHostFactorsFromHost(const TString& host, const TString& owner, NRealTime::THostFactors& proto);
void FillHerfFromHostFactorsProto(const NRealTime::THostFactors& proto, THostErfInfo& herf);

void WriteErf(const TErfCreateConfig& config, TErfs& erfVec, const TString& erFilefName = "");
void WriteErf(const TErfCreateConfig& config, TErfsRemap& erfVec, const TString& erFilefName = "");
void WriteErf2(const TErfCreateConfig& config, TVector<SDocErf2Info>& erfVec);
void PatchErf(const TErfCreateConfig& config, TErfsRemap& erfVec, const TString& erFilefName = "");
void PatchErf2(const TErfCreateConfig& config, TErfs2& erfVec);
void ReadErf(const TErfCreateConfig& config, TErfs* erfVec);
void ReadErf(const TErfCreateConfig& config, TErfsRemap* erfVec);
void ReadErf2(const TErfCreateConfig& config, TVector<SDocErf2Info>* erfVec);
void ReadHerf(const TErfCreateConfig& config, TVector<THostErfInfo>* herfVec);

ui32 ConvertOrangeToRobotHops(ui32 orangeHops);

// binary copy erf from indexeddoc
bool FillErfFromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, SDocErfInfo3& erf);
bool FillErf2FromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, SDocErf2Info& erf);
bool FillHostErfFromIndexedDocBinary(const NRealTime::TIndexedDoc& indexedDoc, THostErfInfo& erf);

struct TBatchRegexpCalcer;
double CalcUrlGskModel(const TString& url, const TBatchRegexpCalcer& urlClassifier);
int CalcUrlGskModel4Erf(const TString& url, const TBatchRegexpCalcer& urlClassifier);

bool IsAccessible(const TErfCreateConfig& config, const TString fileName);

void FillErfAggregatedValues(const TErfAttrs& attrs, SDocErfInfo3& erf, time_t currentTime);
