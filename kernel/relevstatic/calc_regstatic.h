#pragma once

#include <kernel/region2country/countries.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/web_factors_info/factor_names.h>
#include <ysite/yandex/erf_format/host_erf_format.h>
#include <ysite/yandex/erf/erf_compress.h>
#include <library/cpp/on_disk/head_ar/head_ar_2d.h>
#include <kernel/search_types/search_types.h>
#include <kernel/doom/offroad_reg_herf_wad/reg_herf_io.h>
#include <kernel/doom/offroad_reg_erf_wad/reg_erf_io.h>
#include <array>

class TFactorView;
struct TRegHostStaticFactorsAccessor;
class TRegHostStaticFeaturesCalcer;

typedef TArrayWithHead2D<TRegHostErfInfo> TRegHostErf; // indexed by (hostId, region)

//////
//
//
//////

// Use this structure to pass regions
// from DocFeatureCalcer to relev static calcers.
// Regions are organized into fixed number of slots.
//
class TRelevStaticRegions {
public:
    enum ERegionSlot {
        RsCountry,
        RsUserRegion,

        RsNumRegionSlots
    };

public:
    TRelevStaticRegions(TCateg relevCountry) {
        Fill(Regions.begin(), Regions.end(), END_CATEG);

        Regions[RsCountry] = relevCountry;
    }

    void InitLocalRegions(TCateg relevRegion) {
        Regions[RsUserRegion] = relevRegion;
    }

    TCateg GetRegion(ERegionSlot slot) const {
        return Regions[slot];
    }

    TCateg operator[] (ERegionSlot slot) const {
        return GetRegion(slot);
    }

private:
    std::array<TCateg, RsNumRegionSlots> Regions;
};

//////
//
//
//////

class TRegHostErfCache {
private:
    struct TCacheEntry {
        TCateg                  Region = END_CATEG;
        TRegHostErfInfo  Info;
        bool HasData = false;
    };

    using TData = std::array<TCacheEntry, TRelevStaticRegions::RsNumRegionSlots>;

private:
    const TRegHostStaticFeaturesCalcer*  RegHostCalcer;

    ui32                HostId;
    TData               Data;

public:
    TRegHostErfCache(const TRegHostStaticFeaturesCalcer* regHostCalcer)
        : RegHostCalcer(regHostCalcer)
        , HostId(0)
    {
        Y_ASSERT(regHostCalcer);
    }

    // Return result is for shorthand usage, e.g. CalcFeature(cache.UpdateHost(), ...)
    //
    TRegHostErfCache& UpdateHost(ui32 hostId) {
        if (hostId != HostId) {
            Flush();
        }

        HostId = hostId;
        return *this;
    }

    void Flush() {
        Fill(Data.begin(), Data.end(), TCacheEntry());
    }

    const TRegHostErfInfo* AccessSlot(size_t slot, TCateg region, TRegHostErfAccessor* accessor);

    const TRegHostErfInfo* TrySlot(size_t slot, TCateg region) const {
        Y_ASSERT(slot < Data.size());
        if (region != Data[slot].Region || !Data[slot].HasData) {
            return nullptr;
        }

        return &Data[slot].Info;
    }
};

class TRegHostStaticFeaturesCalcer: TNonCopyable {
private:
    TRegHostErf                 RegHostErf;
    bool UseWad = false;
    const NDoom::IWad* ErfWad = nullptr;
    THolder<NDoom::TRegHostErfIo::TSearcher> WadSearcher;

protected:
    TRegHostStaticFeaturesCalcer() = default;

public:
    using TErfAccessor = TRegHostErfAccessor;

    TRegHostStaticFeaturesCalcer(const TString& indexregherf, bool isPolite);
    TRegHostStaticFeaturesCalcer(const TMemoryMap& mapping, bool isPolite);
    TRegHostStaticFeaturesCalcer(const NDoom::IWad* wad);
    TRegHostStaticFeaturesCalcer(bool isPolite, const size_t fileSize, void* ptr, const TString& fileName);
    virtual ~TRegHostStaticFeaturesCalcer() {}

    void CalcFeatures(TRegHostStaticFactorsAccessor& factor,
        TRegHostErfCache& hostErfCache,
        const TRelevStaticRegions& regions,
        TRegHostErfAccessor* accessor) const;

    void CalcFeatures(TFactorView& factor, TRegHostErfCache& hostErfCache, const TRelevStaticRegions& regions, TRegHostErfAccessor* accessor) const;
    void CalcFeaturesNoCache(TFactorView& factor, ui32 hostId, const TRelevStaticRegions& regions, TRegHostErfAccessor* accessor) const;

    THolder<TRegHostErfAccessor> NewErfAccessor() const;
    virtual const TRegHostErfInfo* GetHostErfInfo(ui32 hostId, TCateg region, TRegHostErfAccessor* accessor) const;
    //TODO(lagrunge) delete this stuff
    const TRegHostErf& GetErf() const {
        return RegHostErf;
    }
};

class TOneDocRegHostStaticFeaturesCalcer : public TRegHostStaticFeaturesCalcer
{
private:
    TCateg          RelevCountry;
    TRegHostErfInfo RegHostErfInfo;

public:
    TOneDocRegHostStaticFeaturesCalcer(TCateg relevCountry, const TRegHostErfInfo& regHostErfInfo)
        : RelevCountry(relevCountry)
        , RegHostErfInfo(regHostErfInfo)
        {
        }

    const TRegHostErfInfo* GetHostErfInfo(ui32 hostId, TCateg region, TRegHostErfAccessor* = nullptr) const override {
        Y_UNUSED(hostId);
        return (region == RelevCountry ? &RegHostErfInfo : nullptr);
    }
};

TRegHostStaticFeaturesCalcer* GetRegHostStaticFeaturesCalcer(const TString& indexname, bool isPolite, bool useWad, THolder<NDoom::IWad>& regHerfWad, bool lockMemory);

//////
//
//
//////

// TODO: get rid of base class after moving to IRelevance in imgsearch
class TRegDocStaticFeaturesCalcerBase: TNonCopyable {
public:
    using TErfAccessor = TRegErfAccessor;

    virtual ~TRegDocStaticFeaturesCalcerBase() {};

    void CalcFeatures(TRegDocStaticFactorsAccessor& factor, ui32 docId, const TRelevStaticRegions& regions, TRegErfAccessor* accessor) const;
    void CalcFeatures(TFactorView& factor, ui32 docId, const TRelevStaticRegions& regions, TRegErfAccessor* accessor) const;

    virtual bool GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info, TRegErfAccessor* accessor) const {
        Y_UNUSED(docId);
        Y_UNUSED(region);
        Y_UNUSED(info);
        Y_UNUSED(accessor);
        return false;
    }

    virtual THolder<TRegErfAccessor> NewErfAccessor() const {
        return MakeHolder<TRegErfAccessor>(NDoom::EAccessorState::NotInitilized);
    }
};

class TRegDocStaticSimpleIndexStorage {
public:
    typedef TArrayWithHead2D<TRegErfInfo> TRegErf;

private:
    TRegErf RegErf;

public:
    TRegDocStaticSimpleIndexStorage(const TString& indexregerf, bool isPolite);
    TRegDocStaticSimpleIndexStorage(const TMemoryMap& mapping, bool isPolite);

    bool GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info) const;

    const TRegErf& GetErf() const {
        return RegErf;
    }
};

class TRegDocStaticCompressedIndexStorage {
public:
    typedef TCompressedErf<TRegErfCodec> TRegErf;

private:
    TRegErf RegErf;

public:
    TRegDocStaticCompressedIndexStorage(const TString& indexregerf, bool isPolite);
    TRegDocStaticCompressedIndexStorage(const TMemoryMap& mapping, bool isPolite);

    bool GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info) const;

    const TRegErf& GetErf() const {
        return RegErf;
    }
};

// TODO(lagrunge) Erase with All other index storages
struct TDummyIndexStorage {
    using TRegErf = void*;
    TDummyIndexStorage(const TString&, bool){

    }

    TDummyIndexStorage(const TMemoryMap&, bool) {

    }

    bool GetRegErfInfo(ui32, TCateg, TRegErfInfo&) const {
        return false;
    }
};

template <typename TIndexStorage>
class TRegDocStaticFeaturesCalcer: public TRegDocStaticFeaturesCalcerBase {
public:
    typedef typename TIndexStorage::TRegErf TRegErf;

    TRegDocStaticFeaturesCalcer(const NDoom::IWad* wad)
        : IndexStorage(TString(), false)
    {
        UseWad = true;
        ErfWad = wad;
        WadSearcher.Reset(new NDoom::TRegErfIo::TSearcher(ErfWad));
    }

    TRegDocStaticFeaturesCalcer(const TString& indexregerf, bool isPolite)
        : IndexStorage(indexregerf, isPolite)
    {
    }

    TRegDocStaticFeaturesCalcer(const TMemoryMap& mapping, bool isPolite)
        : IndexStorage(mapping, isPolite)
    {
    }

    inline bool GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info, TRegErfAccessor* accessor) const override {
        if (UseWad) {
            const TRegErfInfo* foundInfo = accessor->GetRegErf(docId, static_cast<ui32>(region));
            if (foundInfo) {
                info = *foundInfo;
                return true;
            }
            return false;
        }

        return IndexStorage.GetRegErfInfo(docId, region, info);
    }

    const TRegErf& GetErf() const {
        return IndexStorage.GetErf();
    }

    THolder<TRegErfAccessor> NewErfAccessor() const override {
        if (UseWad)
            return MakeHolder<TRegErfAccessor>(WadSearcher.Get());
        return TRegDocStaticFeaturesCalcerBase::NewErfAccessor();
    }

private:
    TIndexStorage IndexStorage;
    bool UseWad = false;
    const NDoom::IWad* ErfWad = nullptr;
    THolder<NDoom::TRegErfIo::TSearcher> WadSearcher;
};

class TRegOneDocStaticFeaturesCalcer: public TRegDocStaticFeaturesCalcerBase
{
private:
    TCateg      RelevCountry;
    TRegErfInfo RegErfInfo;
public:
    TRegOneDocStaticFeaturesCalcer(TCateg relevCountry, const TRegErfInfo& regErfInfo)
        : RelevCountry(relevCountry)
        , RegErfInfo(regErfInfo)
        {}

    bool GetRegErfInfo(ui32 docId, TCateg region, TRegErfInfo& info, TRegErfAccessor* = nullptr) const override {
        Y_UNUSED(docId);

        if (region == RelevCountry) {
            info = RegErfInfo;
            return true;
        }

        return false;
    }
};

TRegDocStaticFeaturesCalcerBase* GetRegDocStaticFeaturesCalcer(const TString& indexname, bool isPolite, bool useWad, THolder<NDoom::IWad>& regErfWad);
