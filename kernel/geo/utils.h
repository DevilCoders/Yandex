#pragma once

#include "reg_data.h"

#include <library/cpp/binsaver/bin_saver.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>
#include <util/draft/ip.h>

#include <util/stream/file.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>

// TODO(dieash@) dirty ad-hoc fix for 110+ failed builds
// with the bunch of errors "use of undeclared identifier"
// such as 'JoinVectorIntoString', 'JoinStrings', '[sS]plitStroku'
//
#include <util/string/vector.h>

#include <algorithm>

#include <library/cpp/geobase/lookup.hpp>
#include <geobase/library/binary_reader.hpp>

enum EGeoRegionType
{
    GRT_OTHER = 0,        // Earth, "unknown", "other", "deleted"
    GRT_CONTINENT = 1,    // Europe, Asia, Africa
    GRT_GEOPOLITICAL = 2, // Middle East, CIS, Baltic states
    GRT_COUNTRY = 3,      // Russia, USA, Ukraine, Liberia
    GRT_COUNTRY_PART = 4, // Siberia, Ural, Caucasus
    GRT_REGION = 5,       // Moscow region, Samara region, Tyumen region
    GRT_CITY_TOWN = 6,    // Moscow, Adelaida, Mumbai, Bobruisk
    GRT_VILLAGE = 7,      // Donskoe, Kuznetsovo, Urus-Martan
    GRT_CITY_DISTRICT = 8,// Admiralteyski, Solntsevo, Obolon'
    GRT_SUBWAY_ST = 9,    // Park Kultury, Ozerki, Narvskaya
    GRT_DISTRICT = 10,    // Krasnogorskiy rayon, Pervomayskiy rayon
    GRT_AIRPORT = 11,     // Bolshoe Savino (Perm)
    GRT_EXT_TERRITORY = 12, // Bermuda, etc.
    GRT_CITY_DISTRICT_LVL2 = 13,
    GRT_MONORAIL = 14,
    GRT_VILLAGE_SETTLEMENT = 15,
    GRT_SIZE = 16,        // Change this value when the number of fields is being altered
};

struct TGeoRegionDescr
{
    TGeoRegion Parent = -1;
    TString Name;
    TString EnglishName;
    EGeoRegionType Type = GRT_OTHER;
    TString Timezone;
    TString ShortName;
    TGeoRegion ChiefRegion = 0;
    TMap<TString, TMap<TString, TString> > Names;
    float Latitude = 0;
    float Longitude = 0;

    TGeoRegionDescr() = default;

    SAVELOAD(Parent, Name, EnglishName, Type, Timezone, ShortName, ChiefRegion, Names, Latitude, Longitude);
    Y_SAVELOAD_DEFINE(Parent, Name, EnglishName, Type, Timezone, ShortName, ChiefRegion, Names, Latitude, Longitude);
};

typedef TVector< std::pair<TGeoRegion,TGeoRegion> >     TGeoRegionDFSVector;
typedef THashMap<TGeoRegion, TGeoRegion>           TGeoRegionParentMap;
typedef THashMap<TGeoRegion, TGeoRegionDescr>      TGeoRegionDescrMap;
typedef THashMap<TGeoRegion, TVector<TGeoRegion> > TGeoParent2Children;
typedef THashMap<TGeoRegion, ui32>                TRelevRegionLevelMap;

class TRegionsDB : public IObjectBase
{
    OBJECT_METHODS(TRegionsDB);

private:
    TGeoParent2Children Parent2Children;
    TGeoRegionDFSVector RegionsDFS;
    TGeoRegionDescrMap  DescrMap;

    bool Initialized = false;

public:
    const TGeoRegionDescr& GetDescr(TGeoRegion reg) const
    {
        TGeoRegionDescrMap::const_iterator it = DescrMap.find(reg);
        if (it == DescrMap.end()) {
            static TGeoRegionDescr empty = TGeoRegionDescr();
            return empty;
        }
        return it->second;
    }

    SAVELOAD_OVERRIDE(IObjectBase, Parent2Children, RegionsDFS, DescrMap, Initialized);
    Y_SAVELOAD_DEFINE(Parent2Children, RegionsDFS, DescrMap, Initialized);

public:
    TRegionsDB()
        : Initialized(false)
    {}

    // geodataPath must contain path to either geodata4.bin or geodata5.bin
    explicit TRegionsDB(const TString& geodataPath, bool compatGlobalRegion = true);

    bool IsInitialized() const
    {
        return Initialized;
    }

    const TGeoRegionDFSVector& GetDFSVector() const
    {
        return RegionsDFS;
    }

    TGeoRegion GetParent(TGeoRegion reg) const
    {
        return GetDescr(reg).Parent;
    }

    TGeoRegion GetCountryParent(TGeoRegion reg) const;

    TString GetName(TGeoRegion reg) const
    {
        return GetDescr(reg).Name;
    }

    TString GetNameUilCase(TGeoRegion reg, const TString uil, const TString c) const
    {
        const TMap<TString, TMap<TString, TString> >& names = GetDescr(reg).Names;
        if (const TMap<TString, TString> *mapCases = MapFindPtr(names, uil)) {
            if (const TString* nameUilCase = MapFindPtr(*mapCases, c)) {
                return *nameUilCase;
            }
        }
        return TString();
    }

    TString GetEnglishName(TGeoRegion reg) const
    {
        return GetDescr(reg).EnglishName;
    }

    EGeoRegionType GetType(TGeoRegion reg) const
    {
        return GetDescr(reg).Type;
    }

    TString GetTimezone(TGeoRegion reg) const
    {
        return GetDescr(reg).Timezone;
    }

    TString GetShortName(TGeoRegion reg) const
    {
        return GetDescr(reg).ShortName;
    }

    TGeoRegion GetChiefRegion(TGeoRegion reg) const
    {
        return GetDescr(reg).ChiefRegion;
    }

    const TVector<TGeoRegion>& GetChildren(TGeoRegion reg) const
    {
        static TVector<TGeoRegion> Empty;
        TGeoParent2Children::const_iterator it = Parent2Children.find(reg);
        return it != Parent2Children.end() ? it->second : Empty;
    }
};

bool IsFromParentRegion(const TRegionsDB& regionsDB, TGeoRegion region, TGeoRegion parent);

// region resolver with inclusive-exclusive regions
class TIncExcRegionResolver : public IObjectBase
{
    OBJECT_METHODS(TIncExcRegionResolver);

private:
    typedef THashMap<TGeoRegion, TVector<TGeoRegion> > TReg2Children;
    typedef THashMap<TGeoRegion, TVector<TString> >     TReg2Labels;

    TVector<TString> Empty;
    TReg2Labels     Reg2Labels; // labels are either '225' ... or '225-213' (Russia excl Moscow)

public:
    SAVELOAD_OVERRIDE(IObjectBase, Reg2Labels);
    Y_SAVELOAD_DEFINE(Reg2Labels);

private:
    void AppendToChildren(const TReg2Children& reg2Children,
                          TGeoRegion region, const TString& label,
                          const THashSet<TGeoRegion>& excRegions);

public:

    TIncExcRegionResolver() = default; // for serialization

    TIncExcRegionResolver(const TRegionsDB& regionsDB, const TString& incExcRegionsFile);

    const TVector<TString>& GetRegLabels(TGeoRegion region) const;

    void GetAllRegLabels(THashSet<TString>& labels) const;
};

/* TIterator is slave iterator
 * TDerived is derived master iterator, this parameter is need to implement common functions(currently ++)
 * TDerived must also have static method GetRegion(const TIterator&) -- almost the same thing could be required by making pure virtual function
 * but use of static polymorphism seems better, because there is no virtual function call and non-virtual function has better chances to be inlined by compiler
 */
template <typename TIterator, typename TDerived>
class TRelevRegionIteratorBase
{
private:
    const TRelevRegionLevelMap* LevelMap;
    ui32      CurrentLevel;
    ui32      ToLevel;
    TIterator Begin;
    TIterator End;
    TIterator CurrentIterator;
protected:
    inline void IterateUntilFound(bool setup = false)
    {
        if (!IsValid()) {
            if (setup)
                return;
            ythrow yexception() << "Attempt to iterate past end";
        }
        if (!setup) {
            // after iterator is initialized, CurrentIterator always points to existing element
            ++CurrentIterator;
        }
        for (; CurrentLevel <= ToLevel; ++CurrentLevel) {
            for (; CurrentIterator != End; ++CurrentIterator) {
                TGeoRegion reg = TDerived::GetRegion(CurrentIterator);

                TRelevRegionLevelMap::const_iterator it = LevelMap->find(reg);
                if (it == LevelMap->end())
                    ythrow yexception() << "No such region in TRelevRegionResolver::LevelMap -- " << reg;
                if (it->second == CurrentLevel)
                    return;
            }
            if (CurrentIterator == End)
                CurrentIterator = Begin;
        }
    }

    TRelevRegionIteratorBase(const TRelevRegionLevelMap& levelMap, ui32 fromLevel, ui32 toLevel, const TIterator& begin, const TIterator& end)
      : LevelMap(&levelMap)
      , CurrentLevel(fromLevel)
      , ToLevel(toLevel)
      , Begin(begin)
      , End(end)
      , CurrentIterator(Begin)
    {
        static_assert(std::is_base_of<TRelevRegionIteratorBase, TDerived>::value, "expect TDerivedIsActuallyDerived");
        IterateUntilFound(true);
    }

public:
    typename TIterator::value_type operator*()
    {
        return *CurrentIterator;
    }

    TIterator operator->()
    {
        return CurrentIterator;
    }

    TDerived& operator++()
    {
        IterateUntilFound();
        return *static_cast<TDerived*>(this);
    }

    inline bool IsValid()
    {
        return CurrentLevel <= ToLevel;
    }
};

template <typename TMapIterator>
class TRelevRegionMapIterator : public TRelevRegionIteratorBase< TMapIterator, TRelevRegionMapIterator<TMapIterator> >
{
private:
    static const bool MappedFromTGeoRegion = std::is_same<typename TMapIterator::value_type::first_type, const TGeoRegion>::value;
    static_assert(MappedFromTGeoRegion, "expect MappedFromTGeoRegion");

public:
    TRelevRegionMapIterator(const TRelevRegionLevelMap& levelMap, ui32 fromLevel, ui32 toLevel, const TMapIterator& begin, const TMapIterator& end)
        : TRelevRegionIteratorBase< TMapIterator, TRelevRegionMapIterator<TMapIterator> >(levelMap, fromLevel, toLevel, begin, end)
    {}

    static inline TGeoRegion GetRegion(TMapIterator it)
    {
        return it->first;
    }
};

template <typename TIterator>
class TRelevRegionContainerIterator : public TRelevRegionIteratorBase< TIterator, TRelevRegionContainerIterator<TIterator> >
{

public:
    TRelevRegionContainerIterator(const TRelevRegionLevelMap& levelMap, ui32 fromLevel, ui32 toLevel, const TIterator& begin, const TIterator& end)
        : TRelevRegionIteratorBase< TIterator, TRelevRegionContainerIterator<TIterator> >(levelMap, fromLevel, toLevel, begin, end)
    {}

    static inline TGeoRegion GetRegion(TIterator it)
    {
        return *it;
    }
};

class TRelevRegionResolver : public IObjectBase
{
    OBJECT_METHODS(TRelevRegionResolver);

private:
    TGeoRegionParentMap   RelevRegionsMap;
    TGeoRegionParentMap   RelevCountriesMap;
    THashSet<TGeoRegion> RelevRegions;
    THashSet<TGeoRegion> RelevCountries;
    TGeoRegionDFSVector   RelevRegionsDFS;
    TGeoRegion            DefaultRegion;   // old: 225 - Russia, new 0

    TGeoRegionParentMap   RRChildToParentMap;
    TRelevRegionLevelMap  RelevRegionLevel;
    ui32                  MaxRegionLevel;

public:

    SAVELOAD_OVERRIDE(IObjectBase, RelevRegionsMap, RelevCountriesMap, RelevRegions, RelevCountries, RelevRegionsDFS, DefaultRegion, RRChildToParentMap,
        RelevRegionLevel, MaxRegionLevel);

private:
    void Init(TString geodataPath, const THashSet<TGeoRegion>& relevRegions, TGeoRegion defRegion);

public:
    TRelevRegionResolver() = default; // for serialization

    // geodataPath must contain path to either geodata4.bin or geodata5.bin
    TRelevRegionResolver(TString geodataPath, TString relevRegionsPath,
                         TGeoRegion defRegion = 225);

    // geodataPath must contain path to either geodata4.bin or geodata5.bin
    TRelevRegionResolver(TString geodataPath, const THashSet<TGeoRegion>& relevRegions,
                         TGeoRegion defRegion = 225)
    {
        Init(geodataPath, relevRegions, defRegion);
    }

    explicit operator bool() const
    {
        return !GetDFS().empty();
    }

    TGeoRegion GetRelevRegion(TGeoRegion region) const;
    /// same as GetRelevRegion, but without stupid exceptions
    TGeoRegion GetRelevRegionOrDefault(TGeoRegion region, TGeoRegion defaultRegion = 0) const;

    TGeoRegion GetRelevCountry(TGeoRegion region) const;
    /// same as GetRelevCountry, but without stupid exceptions
    TGeoRegion GetRelevCountryOrDefault(TGeoRegion region, TGeoRegion defaultRegion = 0) const;

    const TGeoRegionDFSVector& GetDFS() const
    {
        return RelevRegionsDFS;
    }

    bool IsKnownRegion(TGeoRegion region) const;

    inline bool IsTopRegion(TGeoRegion region) const
    {
        TRelevRegionLevelMap::const_iterator it = RelevRegionLevel.find(region);
        return it != RelevRegionLevel.end() && it->second == MaxRegionLevel;
    }

    inline TGeoRegion GetRRParent(TGeoRegion region) const
    {
        TGeoRegionParentMap::const_iterator it = RRChildToParentMap.find(region);
        if (it == RRChildToParentMap.end())
            ythrow yexception() << "Region " << region << " is not a relev region";
        return it->second;
    }

    inline ui32 GetMaxRegionLevel()
    {
        return MaxRegionLevel;
    }

    void GetRelevRegionsDFS(TGeoRegion region, TVector<TGeoRegion>& dfs) const;

#define EXPAND_GET_ITERATOR_IMPL(CONTAINER, QUALIFIERS, ITERATOR, METHOD_NAME, RR_ITERATOR) \
    template <class CONTAINER> \
    RR_ITERATOR<typename ITERATOR> METHOD_NAME(QUALIFIERS CONTAINER& map, ui32 fromLevel = 0, ui32 toLevel = -1) const \
    { \
        return RR_ITERATOR<typename ITERATOR>(RelevRegionLevel, fromLevel, std::min(MaxRegionLevel, toLevel), map.begin(), map.end()); \
    } \

    EXPAND_GET_ITERATOR_IMPL(TMapType, ,TMapType::iterator, GetRRMapIterator, TRelevRegionMapIterator)
    EXPAND_GET_ITERATOR_IMPL(TMapType, const, TMapType::const_iterator, GetRRMapIterator, TRelevRegionMapIterator)
    EXPAND_GET_ITERATOR_IMPL(TContainer, ,TContainer::iterator, GetRRContainerIterator, TRelevRegionContainerIterator)
    EXPAND_GET_ITERATOR_IMPL(TContainer, const, TContainer::const_iterator, GetRRContainerIterator, TRelevRegionContainerIterator)

#undef EXPAND_GET_ITERATOR_IMPL
};


// geodataPath must contain path to either geodata4.bin or geodata5.bin
TObj<TRelevRegionResolver> CreateCountryDataRRR(TString geodataPath, TString relevRegionsPath = "",
        TGeoRegion defRegion = 225);


struct TGeoRegionWithIndex
{
    TGeoRegion Region;
    i32 Index;

    explicit TGeoRegionWithIndex(TGeoRegion region, i32 index = -1)
        : Region(region)
        , Index(index)
    {}

    bool IsRelev() const
    {
        return Index != -1 && Region != -1;
    }

    bool operator < (const TGeoRegionWithIndex& rhs) const
    {
        return Index < rhs.Index || (Index == rhs.Index && Region < rhs.Region);
    }
};


// assumed to be used with TRegionDFSOrderedMapping: provides region ids for the last one
// ordering:
//   * id(non relev region) < id(relev region)
//   * id(non relev region) VS id(non relev region) depends on region
//   * id(relev_region1) < id(relev_region2) iff relev_region1 occurs earlier in RegionResolver's DFS
// guarantees same order of regions as RegionResolver's DFS (for relev regions)
class TRegionDFSArranger
{
private:
    const TRelevRegionResolver& RegionResolver;
    // Region2OrderDFS[region] is position of region in DFS vector of relev region or -1 for other regions
    TVector<i32> Region2OrderDFS;

public:
    TRegionDFSArranger(const TRelevRegionResolver& rrr);

    TGeoRegionWithIndex GetRegionId(TGeoRegion reg) const;
    TGeoRegionWithIndex GetRelevParentId(TGeoRegionWithIndex regId) const;
};


template<typename TData>
class TRegionDFSOrderedMapping : public TMap<TGeoRegionWithIndex, TData>
{};


bool IsFromParentRegion(const TRelevRegionResolver &resolver, TGeoRegion region, TGeoRegion parent);

bool IsRelevfmlCountryRegion(TGeoRegion region);

THashSet<TGeoRegion> ParseRelevRegions(TString fileName);
THashSet<TGeoRegion> ParseRelevRegionsFromRawData(const TString& rawData);

// for TIp4 or TIp6, not TIp4Or6
template<class TIp>
struct TIpRange
{
    TIp Start;
    TIp End;

    TIpRange(const TIp& start, const TIp& end)
        : Start(start)
        , End(end)
    {}

    TIpRange(const char * start, const char * end);

    TIpRange() = default;

    SAVELOAD(Start, End);

    bool Has(const TIp& ip) const {
        TIpCompare<TIp> cmp;
        return cmp.LessEqual(Start, ip) && cmp.Less(ip, End);
    }
};


// for TIp4 or TIp6, not TIp4Or6
template<class TIp>
struct TIpToRangeCompare
{
    bool operator()(const TIpRange<TIp>& range, const TIp& ip) const {
        return TIpCompare<TIp>()(range.End, ip);
    }
};


using TGeobaseLookup = NGeobase::TLookup;

const TGeoRegion SPECIAL_IP_REGION = 99999;

// for TIp4 or TIp6, not TIp4Or6
template<class TIp>
struct TIPREG : public IObjectBase
{
    OBJECT_METHODS(TIPREG);

    typedef TVector< TIpRange<TIp> > TIpRanges;

    TIpRanges                 IpRanges;
    TVector<TGeoRegion>       Regions;

    static TIp IpFromIPREGStr(const TStringBuf& ipregIP);

    TMaybe<TGeoRegion> OnlyRegCode;

    static size_t GetGeobaseIPREGsCount(const NGeobase::NDetails::TBinaryReader& binaryReader);

    struct TGeobaseIPREGItem
    {
        TIpRange<TIp> IpRange;
        size_t RegionId;

        TGeobaseIPREGItem(TIpRange<TIp>& ipRange, size_t regionId)
            : IpRange(ipRange)
            , RegionId(regionId)
            {}
    };

    static TGeobaseIPREGItem GetGeobaseIPREGItemByIndex(const NGeobase::NDetails::TBinaryReader& binaryReader, size_t index);

    template<typename Ts, typename Te>
    static void AddIpRange(TIpRanges& ipRanges, Ts& start, Te& end) { ipRanges.push_back(typename TIpRanges::value_type(start, end)); }

    void AddIPREGRecord(TGeoRegion region, const TIpRange<TIp>& ipRange);

public:
    TIPREG() = default;
    TIPREG(const TString& ipreg, const TMaybe<TGeoRegion>& onlyRegCode = TMaybe<TGeoRegion>());
    TIPREG(const NGeobase::NDetails::TBinaryReader& binaryReader, const TMaybe<TGeoRegion>& onlyRegCode = TMaybe<TGeoRegion>());

    SAVELOAD_OVERRIDE(IObjectBase, IpRanges, Regions);

    // returns -1 if not found
    TGeoRegion ResolveRegion(const TIp& ip) const {
        typename TIpRanges::const_iterator it = std::lower_bound(IpRanges.begin(), IpRanges.end(), ip, TIpToRangeCompare<TIp>());
        if ((it == IpRanges.end()) || TIpCompare<TIp>().Less(ip, it->Start)) {
            return -1;
        }
        return Regions[it - IpRanges.begin()];
    }

    size_t GetRangesAmount() const {
        Y_ASSERT(IpRanges.size() == Regions.size());
        return IpRanges.size();
    }
};

template<class TIp>
TIPREG<TIp>::TIPREG(const TString& ipreg, const TMaybe<TGeoRegion>& onlyRegCode)
    : OnlyRegCode(onlyRegCode)
{
    TIFStream in(ipreg);

    TString l;
    TIpRange<TIp> ipRange;
    while (in.ReadLine(l)) {
        TDelimStringIter it(l," ");
        ipRange.Start = TIPREG<TIp>::IpFromIPREGStr(*it);
        ++it;
        ipRange.End = TIPREG<TIp>::IpFromIPREGStr(*it);
        ++it;

        AddIPREGRecord(FromString<TGeoRegion>(*it), ipRange);
    }
}

template <class TIp>
TIPREG<TIp>::TIPREG(const NGeobase::NDetails::TBinaryReader& binaryReader, const TMaybe<TGeoRegion>& onlyRegCode)
    : OnlyRegCode(onlyRegCode)
{
    size_t ipregsCount = GetGeobaseIPREGsCount(binaryReader);
    IpRanges.reserve(ipregsCount);
    Regions.reserve(ipregsCount);

    for (size_t i = 0; i < ipregsCount; ++i) {
        const typename TIPREG<TIp>::TGeobaseIPREGItem& ipregItem = GetGeobaseIPREGItemByIndex(binaryReader, i);
        AddIPREGRecord(ipregItem.RegionId ? ipregItem.RegionId : SPECIAL_IP_REGION, ipregItem.IpRange);
    }
}

template <class TIp>
void TIPREG<TIp>::AddIPREGRecord(TGeoRegion region, const TIpRange<TIp>& ipRange)
{
    if (OnlyRegCode.Defined() && (OnlyRegCode != region))
        return;

    IpRanges.push_back(ipRange);
    Regions.push_back(region);
}

class TIPREG4Or6 : public IObjectBase
{
    OBJECT_METHODS(TIPREG4Or6);

    TIPREG<TIp4> IPREG4;
    TIPREG<TIp6> IPREG6; // Ipv6 is still

public:
    TIPREG4Or6() = default;

    TIPREG4Or6(const TString& ipreg4File, const TMaybe<TGeoRegion>& onlyRegCode = TMaybe<TGeoRegion>())
        : IPREG4(ipreg4File, onlyRegCode)
    {}

    TIPREG4Or6(const TString& ipreg4File, const TString& ipreg6File, const TMaybe<TGeoRegion>& onlyRegCode = TMaybe<TGeoRegion>())
        : IPREG4(ipreg4File, onlyRegCode)
        , IPREG6(ipreg6File, onlyRegCode)
    {}

    TIPREG4Or6(const TGeobaseLookup& gbLookup, const TMaybe<TGeoRegion>& onlyRegCode = TMaybe<TGeoRegion>())
        : IPREG4(*gbLookup.GetBinaryReaderPtr(), onlyRegCode)
        , IPREG6(*gbLookup.GetBinaryReaderPtr(), onlyRegCode)
    {}

    SAVELOAD_OVERRIDE(IObjectBase, IPREG4, IPREG6);

    TGeoRegion ResolveRegion(const TIp4Or6& ip) const;
};


const TGeoRegion YANDEX_REG_CODE     = 9999;

class TCheckIsFromYandex : public IObjectBase
{
    OBJECT_METHODS(TCheckIsFromYandex);

    TIPREG4Or6 IPREG;

public:
    SAVELOAD_OVERRIDE(IObjectBase, IPREG);

    TCheckIsFromYandex() = default;
    TCheckIsFromYandex(const TString& ipregDir);
    TCheckIsFromYandex(const TGeobaseLookup& gbLookup);


    // ip is in inet format
    bool IsFromYandex(const TIp4Or6& ip) const;
    bool IsFromYandex(const char* ipStr) const
    {
        return IsFromYandex(Ip4Or6FromString(ipStr));
    }
};



class TGeoHelper : public IObjectBase
{
    OBJECT_METHODS(TGeoHelper);

    TIPREG4Or6 IPREG;

    TRelevRegionResolver Resolver;

public:
    SAVELOAD_OVERRIDE(IObjectBase, IPREG, Resolver);

    TGeoHelper() = default;
    TGeoHelper(const TString& ipregDir, const TRelevRegionResolver& resolver);
    TGeoHelper(const TGeobaseLookup& gbLookup, const TRelevRegionResolver& resolver);

    TGeoRegion ResolveRegion(const TIp4Or6& ip) const;
    TGeoRegion GetRelevRegion(TGeoRegion userRegion) const;

    const TRelevRegionResolver& GetRelevRegionResolver() const;

    bool IsFromYandex(const TIp4Or6& ip) const;

    bool Validate(TGeoRegion region) const;
    bool ValidateRelev(TGeoRegion region) const;
};


enum EUserType {
    UT_EXTERNAL,
    UT_YANDEX_STAFF,
    UT_YANDEX_SERVER,
    UT_UNKNOWN
};

class TUserTypeResolver : public IObjectBase {
    OBJECT_METHODS(TUserTypeResolver);

    TIPREG4Or6 IPREGYandex;
    TIPREG4Or6 IPSTAFF;

public:
    TUserTypeResolver() = default;

    // TODO(someone@) there are libipreg1 for such kind of tasks
    TUserTypeResolver(const TString& ipregDir)
        : IPREGYandex(ipregDir + LOCSLASH_S + "IPREG.yandex", ipregDir + LOCSLASH_S + "IPv6" + LOCSLASH_S + "IPREG.yandex",
                      YANDEX_REG_CODE)
        , IPSTAFF(ipregDir + LOCSLASH_S + "IPSTAFF", ipregDir + LOCSLASH_S + "IPv6" + LOCSLASH_S + "IPSTAFF")
    {}

    SAVELOAD_OVERRIDE(IObjectBase, IPREGYandex, IPSTAFF);

    EUserType GetUserType(const TIp4Or6& ip) const;
    EUserType GetUserType(const char* ipStr) const;
};
