#include <algorithm>

#include <util/generic/stack.h>
#include <util/stream/file.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>

#include <kernel/country_data/countries.h>

#include "utils.h"


REGISTER_SAVELOAD_CLASS(0x0AF49B04, TRegionsDB);

static const TGeoRegion COMPAT_GLOBAL_REGION = 10000;

inline static void FixCompatRegion(TGeoRegion& region) {
    if (COMPAT_GLOBAL_REGION == region)
        region = 0;
}

// returns section size
size_t SkipToSection(TIFStream& in, const TString& sectionName)
{
    TString prefix = sectionName + ":";

    TString l;
    while (in.ReadLine(l)) {
        if (l.StartsWith(prefix)) {
            TDelimStringIter it(l, ": ");
            ++it;
            size_t sectionSize = 0;
            it.Next(sectionSize);
            return sectionSize;
        }
    }
    ythrow yexception() << "No '" << sectionName << ": ' section ";
    return 0; // for consistency
}

void GetDFS(const TGeoParent2Children& p2c, TGeoRegion reg, TGeoRegionDFSVector& dfs, THashSet<TGeoRegion>& visited)
{
    if (visited.contains(reg))
        ythrow yexception() << "region " << reg << " already visited";
    visited.insert(reg);

    TGeoParent2Children::const_iterator it = p2c.find(reg);
    if (it != p2c.end()) {
        for (TVector<TGeoRegion>::const_iterator it2 = it->second.begin(), eit2 = it->second.end();
             it2 != eit2;
             ++it2)
        {
            GetDFS(p2c, *it2, dfs, visited);
            dfs.push_back(std::make_pair(*it2, reg));
        }
    }
}


TRegionsDB::TRegionsDB(const TString& geodataPath, bool compatGlobalRegion)
    : Initialized(true)
{
    if (geodataPath.StartsWith("debug://")) {
        // Create a dummy RegDB for unit testing purposes
        // List nested regions, separated with a colon:
        // TRegionsDB regDB("debug://10001:225:1:213");
        TStringBuf regions, regStr;
        TStringBuf(geodataPath).AfterPrefix("debug://", regions);
        TGeoRegion parent = -1, region;
        while (regions.NextTok(':', regStr)) {
            region = FromString<TGeoRegion>(regStr);
            Parent2Children[parent].push_back(region);
            auto& descr = DescrMap[region];
            descr = TGeoRegionDescr();
            descr.Parent = parent;
            parent = region;
        }
    } else try {
        TGeobaseLookup gbLookup(geodataPath.data());

        auto regions_ids_list = gbLookup.GetTree(NGeobase::EARTH_REGION_ID);

        // to make more stable structures relying on regions ordering
        Sort(regions_ids_list.begin(), regions_ids_list.end());

        for (const auto id : regions_ids_list) {
            TGeoRegion reg = id;
            TGeoRegion parentReg = gbLookup.GetParentId(id);
            switch (id) {
                case -1:    // deleted
                    continue;
                case COMPAT_GLOBAL_REGION:
                    parentReg = -1;
                    if (compatGlobalRegion) {  // patch for Earth (compatibility)
                        reg = 0;
                    }
                    break;
                default:
                    if (compatGlobalRegion) { // patch for Earth (compatibility)
                        FixCompatRegion(parentReg);
                    }
            }
            if (parentReg != NGeobase::ROOT_ID || reg != NGeobase::ROOT_ID) {
                Parent2Children[parentReg].push_back(reg);
            }

            const auto& regInfo = gbLookup.GetRegionById(reg);
            TGeoRegionDescr& descr = DescrMap[reg];
            {
                descr.Parent      = parentReg;
                descr.Name        = regInfo.GetName();
                descr.EnglishName = regInfo.GetEnName();
                descr.Type        = NGeobase::ERegionType::REMOVED == regInfo.GetEType() ? EGeoRegionType::GRT_OTHER : static_cast<EGeoRegionType>(regInfo.GetType());
                descr.Timezone    = regInfo.GetTimezoneName();
                descr.ShortName   = regInfo.GetStrField("short_en_name");
                descr.ChiefRegion = regInfo.GetCapitalId();
                descr.Latitude    = regInfo.GetLatitude();
                descr.Longitude   = regInfo.GetLongitude();
            }

            TMap<TString, TMap<TString, TString> > names;
            try {
                const char* uils[] = {"ru", "uk", "tr"};
                for (size_t c = 0; c < 3; ++c) {
                    const char* uil = uils[c];
                    const auto& l = gbLookup.GetLinguistics(reg, uil);
                    if (!l.NominativeCase.empty()) names[uil]["nomative"] = l.NominativeCase;
                    if (!l.GenitiveCase.empty())   names[uil]["genitive"] = l.GenitiveCase;
                    if (!l.DativeCase.empty())     names[uil]["dative"]   = l.DativeCase;
                }
            } catch (...) {
                // silently ignore that geobase has no linguistics for this language/region pair
            }
            DescrMap[reg].Names.swap(names);

        }
    } catch (const std::exception& e) {
        ythrow yexception() << "Error while parsing " << geodataPath << ": " << e.what();
    }
    THashSet<TGeoRegion> visited;
    GetDFS(Parent2Children, -1, RegionsDFS, visited);
}

TGeoRegion TRegionsDB::GetCountryParent(TGeoRegion reg) const
{
    while (reg != 0 && reg != -1 && GetType(reg) != GRT_COUNTRY) {
        reg = GetParent(reg);
    }

    if (GetType(reg) == GRT_COUNTRY) {
        return reg;
    } else {
        return 0;
    }
}

bool IsFromParentRegion(const TRegionsDB &regionsDb, TGeoRegion region, TGeoRegion parent)
{
    while (region >= 0) {
        if (region == parent)
            return true;
        region = regionsDb.GetParent(region);
    }
    return false;
}


REGISTER_SAVELOAD_CLASS(0x0AF49B05, TIncExcRegionResolver);


typedef THashMap< TGeoRegion, THashSet<TGeoRegion> > TIncExcMap;

void LoadIncExcMap(const TString& incExcRegionsFile, TIncExcMap& incExcMap)
{
    TIFStream in(incExcRegionsFile);

    TString l;
    while (in.ReadLine(l)) {
        TDelimStringIter it(l,"\t");
        TGeoRegion region;
        it.Next(region);

        if (incExcMap.find(region) != incExcMap.end())
            ythrow yexception() << "Duplicate regions in " << incExcRegionsFile << " file";

        THashSet<TGeoRegion>& excSet = incExcMap[region];

        ++it; // skip name

        while (it.TryNext(region)) {
            excSet.insert(region);
        }
    }
}

TString GetLabel(TGeoRegion incRegion, const THashSet<TGeoRegion>& excRegions)
{
    TStringStream strOut;
    strOut << incRegion;
    for (THashSet<TGeoRegion>::const_iterator it = excRegions.begin(); it != excRegions.end(); ++it) {
        strOut << '-' << *it;
    }
    return strOut.Str();
}

void TIncExcRegionResolver::AppendToChildren(const TIncExcRegionResolver::TReg2Children& reg2Children,
                                             TGeoRegion region, const TString& label,
                                             const THashSet<TGeoRegion>& excRegions)
{
    Reg2Labels[region].push_back(label);
    TReg2Children::const_iterator cit = reg2Children.find(region);
    if (cit != reg2Children.end()) {
        const TVector<TGeoRegion>& children = cit->second;
        for (TVector<TGeoRegion>::const_iterator it = children.begin(), eit = children.end();
             it != eit;
             ++it)
        {
            if (!excRegions.contains(*it))
                AppendToChildren(reg2Children, *it, label, excRegions);
        }
    }
}

TIncExcRegionResolver::TIncExcRegionResolver(const TRegionsDB& regionsDB, const TString& incExcRegionsFile)
{
    TIncExcMap incExcMap;
    LoadIncExcMap(incExcRegionsFile, incExcMap);

    TReg2Children reg2Children;

    const TGeoRegionDFSVector& regs = regionsDB.GetDFSVector();
    for (TGeoRegionDFSVector::const_iterator it = regs.begin(), end = regs.end(); it != end; ++it) {
        TGeoRegion region = it->first;
        if (it->second != -1)
            reg2Children[it->second].push_back(region);
        TIncExcMap::const_iterator ieMapIt = incExcMap.find(region);
        if (ieMapIt != incExcMap.end()) {
            AppendToChildren(reg2Children, region, GetLabel(region, ieMapIt->second), ieMapIt->second);
        }
    }
}

const TVector<TString>& TIncExcRegionResolver::GetRegLabels(TGeoRegion region) const
{
    TReg2Labels::const_iterator it = Reg2Labels.find(region);
    return (it == Reg2Labels.end()) ? Empty : it->second;
}

void TIncExcRegionResolver::GetAllRegLabels(THashSet<TString>& labels) const
{
    labels.clear();
    for (TReg2Labels::const_iterator it = Reg2Labels.begin(), end = Reg2Labels.end(); it != end; ++it) {
        for (TVector<TString>::const_iterator sit = it->second.begin(), esit = it->second.end();
             sit != esit;
             ++sit)
        {
            labels.insert(*sit);
        }
    }
}

static bool IsCountryGlobal(const TRegionsDB& db, TGeoRegion region)
{
    return (db.GetType(region) == GRT_COUNTRY);
}


REGISTER_SAVELOAD_CLASS(0x0AF49B01, TRelevRegionResolver);


void TRelevRegionResolver::Init(TString geodataPath, const THashSet<TGeoRegion>& relevRegions,
                                TGeoRegion defRegion)
{
    TRegionsDB db(geodataPath); // CompatGlobalRegion

    Y_ASSERT(defRegion == 0 || IsCountryGlobal(db, defRegion));

    DefaultRegion = defRegion;

    RelevRegions = relevRegions;
    RelevCountries.clear();
    for (THashSet<TGeoRegion>::const_iterator rrIt = RelevRegions.begin(); rrIt != RelevRegions.end(); ++rrIt) {
        const TCateg& region = *rrIt;
        if (IsCountryGlobal(db, region)) {
            RelevCountries.insert(region);
        }
    }

    const TGeoRegionDFSVector &regs = db.GetDFSVector();
    MaxRegionLevel = 0;

    for (TGeoRegionDFSVector::const_iterator it = regs.begin(), end = regs.end(); it != end; ++it) {
        const TGeoRegion& region = it->first;

        TGeoRegion parent = it->second;

        for (; parent > 0 && !RelevRegions.contains(parent); parent = db.GetParent(parent)) {
        }

        if (RelevRegions.contains(region) || (region == 0)) {
            RelevRegionsDFS.push_back(std::make_pair(region, parent));
            RelevRegionsMap[region] = region;
            RRChildToParentMap[it->first] = parent;

            if (parent >= 0) {
                RelevRegionLevel[parent] = std::max(RelevRegionLevel[parent], RelevRegionLevel[it->first] + 1);
                MaxRegionLevel = std::max(MaxRegionLevel, RelevRegionLevel[parent]);
            }
        } else {
            RelevRegionsMap[region] = parent >= 0 ? parent : DefaultRegion;
        }

        if (IsCountryGlobal(db, region) && RelevCountries.contains(region)) {
            RelevCountriesMap[region] = region;
        } else {
            TGeoRegion countryParent = parent;
            for (; countryParent > 0 && !RelevCountries.contains(countryParent); countryParent = db.GetParent(countryParent)) {
            }

            RelevCountriesMap[region] = countryParent >= 0 ? countryParent : DefaultRegion;
        }
    }
}

THashSet<TGeoRegion> ParseRelevRegions(IInputStream* is) {
    THashSet<TGeoRegion> relevRegions;
    TString l;
    while (is->ReadLine(l)) {
        size_t cPos = l.find('#');
        if (cPos == 0)
            continue;
        if (cPos != TString::npos)
            l.resize(cPos);
        TDelimStringIter it(l,"\t");
        relevRegions.insert(FromString<TGeoRegion>(*it));
    }
    return relevRegions;
}

THashSet<TGeoRegion> ParseRelevRegions(TString fileName) {
    TFileInput rrIn(fileName);
    return ParseRelevRegions(&rrIn);
}

THashSet<TGeoRegion> ParseRelevRegionsFromRawData(const TString& rawData) {
    TStringInput strIn(rawData);
    return ParseRelevRegions(&strIn);
}

TRelevRegionResolver::TRelevRegionResolver(TString geodataPath, TString relevRegionsPath,
                                           TGeoRegion defRegion)
{
    Init(geodataPath, ParseRelevRegions(relevRegionsPath), defRegion);
}

TGeoRegion TRelevRegionResolver::GetRelevRegion(TGeoRegion region) const
{
    FixCompatRegion(region);
    TGeoRegionParentMap::const_iterator it = RelevRegionsMap.find(region);
    Y_ENSURE(it != RelevRegionsMap.end(), "Unknown region " << region);

    // 0 is a top region, no special relevance regions
    return (it->second == 0) ? DefaultRegion : it->second;
}

/*
This code is written to handle logic like that:
    TCateg country = 0;
    try {
        country = RRResolver.GetRelevRegion(region);
    } catch (const yexception&) {
    }

Now we write just
    TCateg country = RRResolver.GetRelevRegionOrDefault(region);
*/
TGeoRegion TRelevRegionResolver::GetRelevRegionOrDefault(TGeoRegion region, TGeoRegion defaultRegion) const
{
    FixCompatRegion(region);
    const TGeoRegion* result = RelevRegionsMap.FindPtr(region);
    if (!result)
        return defaultRegion; // this is not a typo

    // 0 is a top region, no special relevance regions
    return (*result == 0) ? DefaultRegion : *result;
}

TGeoRegion TRelevRegionResolver::GetRelevCountry(TGeoRegion region) const
{
    FixCompatRegion(region);
    const TGeoRegion* result = RelevCountriesMap.FindPtr(region);
    Y_ENSURE(result, "Unknown region " << region);

    // 0 is a top region, no special relevance regions
    return (*result == 0) ? DefaultRegion : *result;
}

TGeoRegion TRelevRegionResolver::GetRelevCountryOrDefault(TGeoRegion region, TGeoRegion defaultRegion) const
{
    FixCompatRegion(region);
    TGeoRegionParentMap::const_iterator it = RelevCountriesMap.find(region);
    if (it == RelevCountriesMap.end())
        return defaultRegion;

    // 0 is a top region, no special relevance regions
    return (it->second == 0) ? DefaultRegion : it->second;
}

bool TRelevRegionResolver::IsKnownRegion(TGeoRegion region) const
{
    FixCompatRegion(region);
    return RelevRegionsMap.contains(region);
}

void TRelevRegionResolver::GetRelevRegionsDFS(TGeoRegion region, TVector<TGeoRegion>& dfs) const
{
    FixCompatRegion(region);
    dfs.clear();
    TGeoRegionParentMap::const_iterator pit = RelevRegionsMap.find(region);
    if (pit == RelevRegionsMap.end())
        ythrow yexception() << "Unknown region " <<  region;
    region = pit->second;

    for (TGeoRegionDFSVector::const_iterator it = RelevRegionsDFS.begin();
         it != RelevRegionsDFS.end();
         ++it)
    {
        if (it->first == region) {
            dfs.push_back(region);
            region = it->second;
            if (region == 0) {
                dfs.push_back(region);
                return;
            }
        }
    }
}

TObj<TRelevRegionResolver> CreateCountryDataRRR(TString geodataPath, TString relevRegionsPath,
        TGeoRegion defRegion) {
    THashSet<TGeoRegion> regions;
    if (relevRegionsPath) {
        regions = ParseRelevRegions(relevRegionsPath);
    }
    for (const auto& country : ::GetRelevCountries()) {
        regions.insert(country);
    }
    return new TRelevRegionResolver(geodataPath, regions, defRegion);
}

TRegionDFSArranger::TRegionDFSArranger(const TRelevRegionResolver& rrr)
    : RegionResolver(rrr)
{
    const TGeoRegionDFSVector& dfs = RegionResolver.GetDFS();
    for (size_t i = 0; i < dfs.size(); ++i) {
        TGeoRegion region = dfs[i].first;
        if (Region2OrderDFS.ysize() <= region)
            Region2OrderDFS.resize(region + 1, -1);
        Region2OrderDFS[region] = i;
    }
}

TGeoRegionWithIndex TRegionDFSArranger::GetRegionId(TGeoRegion reg) const
{
    TGeoRegionWithIndex id(reg);
    if (0 <= reg && reg < Region2OrderDFS.ysize())
        id.Index = Region2OrderDFS[reg];
    return id;
}

TGeoRegionWithIndex TRegionDFSArranger::GetRelevParentId(TGeoRegionWithIndex regId) const
{
    if (regId.IsRelev()) {
        const TGeoRegionDFSVector& dfs = RegionResolver.GetDFS();
        // relev region always has relev parent
        return GetRegionId(dfs[Region2OrderDFS[regId.Region]].second);
    } else {
        return GetRegionId(RegionResolver.GetRelevRegion(regId.Region));
    }
}

bool IsRelevfmlCountryRegion(TGeoRegion region)
{
    return IsCountry((TCateg)region) || (region == 0);
}

bool IsFromParentRegion(const TRelevRegionResolver &resolver, TGeoRegion region, TGeoRegion parent) {
    if (!resolver.IsKnownRegion(region))
        return false;
    TVector<TGeoRegion> dfs;
    resolver.GetRelevRegionsDFS(region, dfs);
    for (int n = 0; n < dfs.ysize(); ++n)
        if (dfs[n] == parent)
            return true;
    return false;
}


REGISTER_SAVELOAD_TEMPL1_CLASS(0x0AF49B07, TIPREG, TIp4);
REGISTER_SAVELOAD_TEMPL1_CLASS(0x0AF49B08, TIPREG, TIp6);


template<>
TIpRange<TIp4>::TIpRange(const char* start, const char* end)
    : Start(IpFromString(start))
    , End(IpFromString(end))
    {}

template<>
TIpRange<TIp6>::TIpRange(const char* start, const char* end)
    : Start(Ip6FromString(start))
    , End(Ip6FromString(end))
    {}

template<>
TIp4 TIPREG<TIp4>::IpFromIPREGStr(const TStringBuf& ipregIP) {
    return HostToInet(FromString<ui32>(ipregIP));
}

template<>
TIp6 TIPREG<TIp6>::IpFromIPREGStr(const TStringBuf& ipregIP) {
    TString ipregIPStr(ipregIP);
    return Ip6FromString(ipregIPStr.data());
}

template<>
size_t TIPREG<TIp4>::GetGeobaseIPREGsCount(const NGeobase::NDetails::TBinaryReader& binaryReader)
{
    return binaryReader.ipregsv4_count();
}

template<>
size_t TIPREG<TIp6>::GetGeobaseIPREGsCount(const NGeobase::NDetails::TBinaryReader& binaryReader)
{
    return binaryReader.ipregsv6_count();
}

template <>
TIPREG<TIp4>::TGeobaseIPREGItem TIPREG<TIp4>::GetGeobaseIPREGItemByIndex(const NGeobase::NDetails::TBinaryReader& binaryReader, size_t index)
{
    const auto& item = binaryReader.ipv4_full_range_by_idx(index);

    TIpRange<TIp4> ipRange = TIpRange<TIp4>(
            HostToInet(item.addr_lo),
            HostToInet(item.addr_hi)
        );
    return TGeobaseIPREGItem(ipRange, item.region_id);
}

template <>
TIPREG<TIp6>::TGeobaseIPREGItem TIPREG<TIp6>::GetGeobaseIPREGItemByIndex(const NGeobase::NDetails::TBinaryReader& binaryReader, size_t index)
{
    const auto& item = binaryReader.ipv6_full_range_by_idx(index);

    TIpRange<TIp6> ipRange;
    memcpy(ipRange.Start.Data, &item.addr_lo, 16);
    memcpy(ipRange.End.Data, &item.addr_hi, 16);

    return TGeobaseIPREGItem(ipRange, item.region_id);
}


REGISTER_SAVELOAD_CLASS(0x0AF49B09, TIPREG4Or6);


TGeoRegion TIPREG4Or6::ResolveRegion(const TIp4Or6& ip) const
{
    if (std::holds_alternative<TIp6>(ip)) {
        return IPREG6.ResolveRegion(std::get<TIp6>(ip));
    } else {
        return IPREG4.ResolveRegion(std::get<TIp4>(ip));
    }
}


REGISTER_SAVELOAD_CLASS(0x0AF49B02, TCheckIsFromYandex);


// TODO(someone@) what about IPREG.json?
TCheckIsFromYandex::TCheckIsFromYandex(const TString& ipregDir)
    : IPREG(ipregDir + LOCSLASH_S + "IPREG.detailed", ipregDir + LOCSLASH_S + "IPv6" + LOCSLASH_S + "IPREG.detailed",
            YANDEX_REG_CODE)
{}

TCheckIsFromYandex::TCheckIsFromYandex(const TGeobaseLookup& gbLookup)
    : IPREG(gbLookup, YANDEX_REG_CODE)
{}

bool TCheckIsFromYandex::IsFromYandex(const TIp4Or6& ip) const
{
    return IPREG.ResolveRegion(ip) != -1;
}

REGISTER_SAVELOAD_CLASS(0x0AF49B03, TGeoHelper);

TGeoHelper::TGeoHelper(const TString& ipregDir, const TRelevRegionResolver &resolver)
    : IPREG(ipregDir + LOCSLASH_S + "IPREG.detailed", ipregDir + LOCSLASH_S + "IPv6" + LOCSLASH_S + "IPREG.detailed")
    , Resolver(resolver)
{}

TGeoHelper::TGeoHelper(const TGeobaseLookup& gbLookup, const TRelevRegionResolver &resolver)
    : IPREG(gbLookup)
    , Resolver(resolver)
{}


TGeoRegion TGeoHelper::ResolveRegion(const TIp4Or6& ip) const
{
    return IPREG.ResolveRegion(ip);
}

TGeoRegion TGeoHelper::GetRelevRegion(TGeoRegion userRegion) const
{
    return Resolver.GetRelevRegion(userRegion);
}

const TRelevRegionResolver& TGeoHelper::GetRelevRegionResolver() const
{
    return Resolver;
}

bool TGeoHelper::Validate(TGeoRegion region) const
{
    Y_UNUSED(region);
    // TODO: invent real validation or get rid of the method
    return true;
}

bool TGeoHelper::IsFromYandex(const TIp4Or6& ip) const
{
    return ResolveRegion(ip) == YANDEX_REG_CODE;
}

bool TGeoHelper::ValidateRelev(TGeoRegion region) const
{
    Y_UNUSED(region);
    //    if(region == 187 || region == )
    //    return false;
    return true;
}


REGISTER_SAVELOAD_CLASS(0x0AF49B06, TUserTypeResolver);


EUserType TUserTypeResolver::GetUserType(const TIp4Or6& ip) const {
    if (IPREGYandex.ResolveRegion(ip) != -1) {
        if (IPSTAFF.ResolveRegion(ip) != -1) {
            return UT_YANDEX_STAFF;
        } else {
            return UT_YANDEX_SERVER;
        }
    } else {
        return UT_EXTERNAL;
    }
}

EUserType TUserTypeResolver::GetUserType(const char* ipStr) const {
    return GetUserType(Ip4Or6FromString(ipStr));
}

