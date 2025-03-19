#include "calc_regstatic.h"

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/region2country/countries.h>
#include <kernel/index_mapping/index_mapping.h>
#include <kernel/searchlog/errorlog.h>
#include <ysite/yandex/relevance/util.h>
#include <util/folder/dirut.h>


const TRegHostErfInfo* TRegHostErfCache::AccessSlot(size_t slot, TCateg region, TRegHostErfAccessor* accessor)
{
    Y_ASSERT(slot < Data.size());
    if (region == END_CATEG) {
        return nullptr;
    }

    const TRegHostErfInfo* info = TrySlot(slot, region);

    if (info) {
        return info;
    }

    Data[slot].Region = region;
    info = RegHostCalcer->GetHostErfInfo(HostId, region, accessor);
    if (info) {
        Data[slot].Info = *info;
        Data[slot].HasData = true;
        return &Data[slot].Info;
    } else {
        Data[slot].HasData = false;
        return nullptr;
    }
}

TRegHostStaticFeaturesCalcer::TRegHostStaticFeaturesCalcer(const TString& indexregherf, bool isPolite)
    : RegHostErf(indexregherf.data(), isPolite)
{
}

TRegHostStaticFeaturesCalcer::TRegHostStaticFeaturesCalcer(const TMemoryMap& mapping, bool isPolite)
    : RegHostErf(mapping, isPolite)
{
}

TRegHostStaticFeaturesCalcer::TRegHostStaticFeaturesCalcer(const NDoom::IWad* wad) {
    UseWad = true;
    ErfWad = wad;
    WadSearcher.Reset(new NDoom::TRegHostErfIo::TSearcher(ErfWad));
}

TRegHostStaticFeaturesCalcer::TRegHostStaticFeaturesCalcer(bool isPolite, const size_t fileSize, void* ptr, const TString& fileName) {
    RegHostErf.Load(isPolite, false, fileSize, ptr, fileName);
}

void TRegHostStaticFeaturesCalcer::CalcFeatures(TRegHostStaticFactorsAccessor& factor,
    TRegHostErfCache& hostErfCache, const TRelevStaticRegions& regions, TRegHostErfAccessor* accessor) const
{
    const TRegHostErfInfo* hostCntErf = hostErfCache.AccessSlot(TRelevStaticRegions::RsCountry, regions[TRelevStaticRegions::RsCountry], accessor);

    if (!hostCntErf) {
        return;
    }

    // Country features
    //
    factor.RegHostRank = Ui82Float(hostCntErf->RegionalHostRank);
    factor.RegIsWiki = BOOL_TO_FLOAT(hostCntErf->IsWikipedia);
    factor.NewsAgencyRating = hostCntErf->RegNewsAgencyRating;

    // Userdata features
    //
    factor.OwnerClicksPCTR_Reg = Ui82Float(hostCntErf->OwnerClicksPCTR);
    factor.OwnerSDiffClickEntropy_Reg = Ui82Float(hostCntErf->OwnerSDiffClickEntropy);
    factor.OwnerSDiffShowEntropy_Reg = Ui82Float(hostCntErf->OwnerSDiffShowEntropy);
    factor.OwnerSDiffCSRatioEntropy_Reg = Ui82Float(hostCntErf->OwnerSDiffCSRatioEntropy);
    factor.OwnerSessNormDuration_Reg = Ui82Float(hostCntErf->OwnerNormDurRate);
    factor.OwnerSatisfied4Rate_Reg = Ui82Float(hostCntErf->OwnerSatisfied4Rate);
    factor.OwnerCTRWithNextPageClicksP10 = Ui82Float(hostCntErf->OwnerCTRWithNextPageClicksP10);
    factor.BrowserHostCntDwellTimeLog = Ui82Float(hostCntErf->RegHostDwellTimeLog);

    factor.CommercialOwnerRank_Reg = Ui42Float(hostCntErf->CommercialOwnerRank);

    factor.BeastHostMeanPos = Ui82Float(hostCntErf->BeastMeanPos);
    factor.BeastHostNumQueries = Ui82Float(hostCntErf->BeastNumQueries);

    factor.YabarHostBrowseRank_Reg = Ui82Float(hostCntErf->YabarHostBrowseRank);

    // Regional features
    //
    const TRegHostErfInfo* hostRegErf = hostErfCache.AccessSlot(TRelevStaticRegions::RsUserRegion, regions[TRelevStaticRegions::RsUserRegion], accessor);

    if (hostRegErf) {
        factor.BrowserHostDwellTimeRegionFrc = Ui82Float(hostRegErf->RegHostDwellTimeFrc);
    }
}

const TRegHostErfInfo* TRegHostStaticFeaturesCalcer::GetHostErfInfo(ui32 hostId, TCateg relevCountry, TRegHostErfAccessor* accessor) const
{
    if (UseWad) {
        return accessor->GetRegErf(hostId, static_cast<ui32>(relevCountry));
    }
    return RegHostErf.Find(hostId, relevCountry);
}

THolder<TRegHostErfAccessor> TRegHostStaticFeaturesCalcer::NewErfAccessor() const {
    if (UseWad)
        return THolder(new TRegHostErfAccessor(WadSearcher.Get()));
    return THolder(new TRegHostErfAccessor(NDoom::EAccessorState::NotInitilized));
}

void TRegHostStaticFeaturesCalcer::CalcFeatures(TFactorView& factor,
    TRegHostErfCache& hostErfCache, const TRelevStaticRegions& regions, TRegHostErfAccessor* accessor) const
{
    // used in robot
    TProtectedFactorStorage protectedFactor(factor);
    return CalcFeatures(protectedFactor.GetRegHostStaticGroup(), hostErfCache, regions, accessor);
}

void TRegHostStaticFeaturesCalcer::CalcFeaturesNoCache(TFactorView& factor,
   ui32 hostId, const TRelevStaticRegions& regions, TRegHostErfAccessor* accessor) const
{
    TRegHostErfCache hostErfCache(this);
    hostErfCache.UpdateHost(hostId);
    TProtectedFactorStorage protectedFactor(factor);
    CalcFeatures(protectedFactor.GetRegHostStaticGroup(), hostErfCache, regions, accessor);
}

TRegHostStaticFeaturesCalcer* GetRegHostStaticFeaturesCalcer(const TString& indexName,
                                                             bool isPolite,
                                                             bool useRegHostErfWad,
                                                             THolder<NDoom::IWad>& regHerfWad,
                                                             bool lockMemory)
{
    TString indexRegHerf = TString::Join(indexName, "regherf");

    if (useRegHostErfWad) {
        TString indexRegHerfWad = TString::Join(indexRegHerf, ".wad");
        if (NFs::Exists(indexRegHerfWad)) {
            regHerfWad = NDoom::IWad::Open(indexRegHerfWad, lockMemory);
            SEARCH_INFO << "Loading wad regherf from " << indexRegHerfWad;
            return new TRegHostStaticFeaturesCalcer(regHerfWad.Get());
        }
        SEARCH_ERROR << "RegHostErfWad index loading failed. Fallback to plain regherf";
    }

    if (!NFs::Exists(indexRegHerf)) {
        SEARCH_ERROR << indexRegHerf << " not loaded, file missing";
        return nullptr;
    }

    SEARCH_INFO << "Loading plain regerf from " << indexRegHerf;
    const TMemoryMap* herfMapping = GetMappedIndexFile(indexRegHerf);
    if (herfMapping) {
        return new TRegHostStaticFeaturesCalcer(*herfMapping, isPolite);
    }
    else {
        return new TRegHostStaticFeaturesCalcer(indexRegHerf, isPolite);
    }
}

void TRegDocStaticFeaturesCalcerBase::CalcFeatures(TRegDocStaticFactorsAccessor& factor, ui32 docId,
    const TRelevStaticRegions& regions, TRegErfAccessor* accessor) const {

    // Country features
    //
    TRegErfInfo cntErf;
    TCateg relevCountry = regions[TRelevStaticRegions::RsCountry];

    if (relevCountry == END_CATEG || !GetRegErfInfo(docId, relevCountry, cntErf, accessor)) {
        return;
    }

    // Userdata features
    //
    factor.UrlQueryVariety_Reg = Ui82Float(cntErf.UrlQueryVariety);
    factor.UrlSessNormDurRate_Reg = Ui82Float(cntErf.UrlSessNormDurRate);
    factor.YabarUrlVisits_Reg = Ui82Float(cntErf.YabarUrlVisits);
    factor.UrlShowsWithNextPageClicksP1 = Ui82Float(cntErf.ShowsWithNextPageClicksP1);
    factor.UrlShowsWithNextPageClicksP10 = Ui82Float(cntErf.ShowsWithNextPageClicksP10);
    factor.UrlQueryTrigramsStatic = Ui82Float(cntErf.UrlQueryTrigramsStatic);
    factor.NHopChainsCountFrc = Ui82Float(cntErf.NHopChainsCountFrc);
    factor.NHopIsFinal = Ui82Float(cntErf.NHopIsFinal);
    factor.VisitsFromWiki = Ui82Float(cntErf.VisitsFromWiki);
    factor.RegBrowserUserHub = Ui82Float(cntErf.UserHub);
    factor.USLongPeriodUrlCtrReg = Ui82Float(cntErf.USLongPeriodCtrReg);
    factor.USLongPeriodUrlDt3600AvgReg = Ui82Float(cntErf.USLongPeriodDt3600AvgReg);
    factor.USLongPeriodUrlLongClickProbReg = Ui82Float(cntErf.USLongPeriodLongClickProbReg);
    factor.USLongPeriodUrlPositionAvgReg = Ui82Float(cntErf.USLongPeriodPositionAvgReg);
    factor.USLongPeriodUrlShowsReg = Ui82Float(cntErf.USLongPeriodShowsReg);
    factor.USLongPeriodUrlMobileDt3600AvgReg = Ui82Float(cntErf.USLongPeriodMobileDt3600AvgReg);
    factor.USLongPeriodUrlMobileDt180AvgReg = Ui82Float(cntErf.USLongPeriodMobileDt180AvgReg);
    factor.UBLongPeriodSearchPercentEndReg = Ui82Float(cntErf.UBLongPeriodSearchPercentEndReg);
    factor.UBLongPeriodLeavesCntReg = Ui82Float(cntErf.UBLongPeriodLeavesCntReg);
    factor.UBLongPeriodDtUrlHChildrenCut600Reg = Ui82Float(cntErf.UBLongPeriodDtUrlHChildrenCut600Reg);

    factor.BeastUrlMeanPos = Ui82Float(cntErf.BeastUrlMeanPos);
    factor.BeastUrlNumQueries = Ui82Float(cntErf.BeastUrlNumQueries);

    // Regional features
    //
    TRegErfInfo regErf;
    TCateg relevUserRegion = regions[TRelevStaticRegions::RsUserRegion];

    if (relevUserRegion != END_CATEG && GetRegErfInfo(docId, relevUserRegion, regErf, accessor)) {
        factor.BrowserUrlDwellTimeRegionFrc = Ui82Float(regErf.BrowserUrlDwellTimeRegionFrc);
    }
}

void TRegDocStaticFeaturesCalcerBase::CalcFeatures(TFactorView& factor, ui32 docId,
    const TRelevStaticRegions& regions, TRegErfAccessor* accessor) const {
    // used in robot
    TProtectedFactorStorage protectedFactor(factor);
    CalcFeatures(protectedFactor.GetRegDocStaticGroup(), docId, regions, accessor);
};

TRegDocStaticSimpleIndexStorage::TRegDocStaticSimpleIndexStorage(const TString& indexregerf, bool isPolite)
    : RegErf(indexregerf, isPolite)
{
}

TRegDocStaticSimpleIndexStorage::TRegDocStaticSimpleIndexStorage(const TMemoryMap& mapping, bool isPolite)
    : RegErf(mapping, isPolite)
{
}

bool TRegDocStaticSimpleIndexStorage::GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info) const
{
    const TRegErfInfo* result = RegErf.Find(docId, region);

    if (result) {
        info = *result;
        return true;
    }

    return false;
}

TRegDocStaticCompressedIndexStorage::TRegDocStaticCompressedIndexStorage(const TString& indexregerf, bool isPolite)
    : RegErf(indexregerf)
{
    Y_UNUSED(isPolite);
}

TRegDocStaticCompressedIndexStorage::TRegDocStaticCompressedIndexStorage(const TMemoryMap& mapping, bool isPolite)
    : RegErf(mapping)
{
    Y_UNUSED(isPolite);
}

bool TRegDocStaticCompressedIndexStorage::GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info) const
{
    if (Y_UNLIKELY(docId >= RegErf.GetDocCount())) {
        return false;
    }

    return RegErf.Find(docId, region, info);
}


TRegDocStaticFeaturesCalcerBase* GetRegDocStaticFeaturesCalcer(const TString& indexname, bool isPolite, bool useWad, THolder<NDoom::IWad>& regErfWad) {
    TString indexregerf = TString::Join(indexname, "regerf");

    if (useWad) {
        TString indexRegErfWad = TString::Join(indexregerf, ".wad");
        if (NFs::Exists(indexRegErfWad)) {
            regErfWad = NDoom::IWad::Open(indexRegErfWad);
            SEARCH_INFO << "Loading wad regerf from " << indexRegErfWad;
            return new TRegDocStaticFeaturesCalcer<TDummyIndexStorage>(regErfWad.Get());
        }
        SEARCH_ERROR << "RegErfWad index loading failed. Fallback to plain regherf";
    }

    TString indexregerfcomp = TString::Join(indexname, "regerfcomp");
    TString index = NFs::Exists(indexregerfcomp) ? indexregerfcomp : indexregerf;
    bool compressed = (index == indexregerfcomp);

    if (!NFs::Exists(index)) {
        return nullptr;
    }

    const TMemoryMap* regErfMapping = GetMappedIndexFile(index);

    if (regErfMapping) {
        if (compressed) {
            return new TRegDocStaticFeaturesCalcer<TRegDocStaticCompressedIndexStorage>(*regErfMapping, isPolite);
        }
        else {
            return new TRegDocStaticFeaturesCalcer<TRegDocStaticSimpleIndexStorage>(*regErfMapping, isPolite);
        }
    }
    else {
        if (compressed) {
            return new TRegDocStaticFeaturesCalcer<TRegDocStaticCompressedIndexStorage>(index, isPolite);
        }
        else {
            return new TRegDocStaticFeaturesCalcer<TRegDocStaticSimpleIndexStorage>(index, isPolite);
        }
    }
}
