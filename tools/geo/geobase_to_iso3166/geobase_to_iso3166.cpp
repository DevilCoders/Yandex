#include <library/cpp/geobase/lookup.hpp>

#include <util/charset/utf8.h>

#include <library/cpp/deprecated/split/delim_string_iter.h>

#include <library/cpp/deprecated/prog_options/prog_options.h>

#include <util/generic/hash.h>

#include <util/stream/output.h>
#include <util/stream/file.h>

#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>


inline TStringBuf DeQuote(const TStringBuf& quoted) {
    return quoted.SubStr(1, quoted.size()-2);
}

template<class TStr>
TString NormRegionName(const TStr& regName)
{
    TString rn = TString(regName);
    rn = StripInPlace(rn);
    SubstGlobal(rn, '-', ' ');
    rn = ToLowerUTF8(rn);
    return rn;
}

template<class TStr>
void AddToName2Ids(THashMap<TString, TVector<i32>>& name2Ids, const TStr& name, i32 regId)
{
    TString normName = NormRegionName(name);
    if (!normName.empty()) {
        name2Ids[normName].push_back(regId);
    }
}


int main(int argc, const char **argv) {
    TProgramOptions progOptions("|geodata|+|iso_countries|+|iso_regions|+|dst_geodata_dump|+|manual_geo2iso|+|dst_geo2iso|+");
    progOptions.Init(argc, argv);

    THashMap<TString, TVector<i32>> name2Ids;

    NGeobase::TLookup gbLookup(progOptions.GetReqOptVal("geodata"));
    {
        for (const auto& regId : gbLookup.GetTree(NGeobase::EARTH_REGION_ID)) {
            const auto& regInfo = gbLookup.GetRegionById(regId);

            AddToName2Ids(name2Ids, regInfo.GetName(), regId);
            AddToName2Ids(name2Ids, regInfo.GetEnName(), regId);

            for (TDelimStringIter it(regInfo.GetSynonyms(), ","); it.Valid(); ++it) {
                AddToName2Ids(name2Ids, *it, regId);
            }
        }
    }

    {
        TOFStream dumpF(progOptions.GetReqOptVal("dst_geodata_dump"));
        for (const auto& e : name2Ids) {
            dumpF << e.first << " :";
            for (const auto& id : e.second) {
                dumpF << ' ' << id;
            }
            dumpF << Endl;
        }
    }

    size_t found = 0;
    size_t notFound = 0;
    size_t amb = 0;
    size_t preset = 0;

    THashMap<i32, TString> manualGeo2ISO;
    THashSet<TString> manualISO;

    // including manual
    THashMap<TString, i32> country2RegId;


    TOFStream dst(progOptions.GetReqOptVal("dst_geo2iso"));

    {
        TIFStream manualF(progOptions.GetReqOptVal("manual_geo2iso"));
        TString l;
        while (manualF.ReadLine(l)) {
            dst << l << Endl;

            i32 regId;
            TString isoCode;
            TDelimStringIter(l, "\t").Next(regId).Next(isoCode);
            manualGeo2ISO[regId] = isoCode;
            manualISO.insert(isoCode);

            if (gbLookup.GetCountryId(regId) == regId) {
                country2RegId[isoCode] = regId;
            }
        }
    }

    THashSet<TString> checkedCountries;

    {
        // CountryA2,CountryName,ISO,Region,RegionAlt,RegionAlt2
        TIFStream iso31662(progOptions.GetReqOptVal("iso_regions"));
        TString l;
        iso31662.ReadLine(l); // skip header
        while (iso31662.ReadLine(l)) {
            TVector<TStringBuf> els;
            StringSplitter(l).Split(',').AddTo(&els);

            // country
            TStringBuf countryIsoCode = DeQuote(els[0]);

            i32 countryRegId = 0;

            if (auto* v = country2RegId.FindPtr(countryIsoCode)) {
                countryRegId = *v;
            } else if (!checkedCountries.contains(countryIsoCode)) {
                TString countryName = NormRegionName(DeQuote(els[1]));

                if (auto* v = name2Ids.FindPtr(countryName)) {
                    TVector<i32> regIds;
                    for (const auto& regId: *v) {
                        if (gbLookup.GetCountryId(regId) == regId)
                            regIds.push_back(regId);
                    }

                    if (regIds.size() > 1) {
                        Cerr << "AmbCountry: " << countryName << ": ";
                        for (const auto& regId: regIds) {
                            Cerr << " " << regId;
                        }
                        Cerr << Endl;
                        //++amb;
                    } else {
                        countryRegId = *(v->begin());
                        country2RegId[countryIsoCode] = countryRegId;
                        //++found;
                        dst << countryRegId << '\t' << countryIsoCode << Endl;
                    }
                }
                checkedCountries.insert(TString(countryIsoCode));
            }

            TStringBuf isoCode = DeQuote(els[2]);

            if (manualISO.contains(isoCode)) {
                ++preset;
                continue;
            }

            THashMap<i32, size_t> regIds;

            for (size_t i = 3; i < 5; ++i) {
                TString name = NormRegionName(DeQuote(els[i]));
                if (name.empty())
                    continue;

                if (auto* v = name2Ids.FindPtr(name)) {
                    for (const auto& regId : *v) {
                        if ((countryRegId == 0) || (gbLookup.IsIdInRegion(regId, countryRegId)))
                            ++regIds[regId];
                    }
                }
            }

            if (regIds.empty()) {
                ++notFound;

                Cerr << "Not found: " << l << Endl;
            } else if (regIds.size() > 1) {
                if (regIds.size() == 2) {
                    auto it = regIds.cbegin();
                    i32 reg1 = it->first;
                    ++it;
                    i32 reg2 = it->first;

                    if (gbLookup.IsIdInRegion(reg1, reg2)) {
                        ++found;

                        Cerr << "AmbResolved: " << l << " : " << reg1 << " in " << reg2 << Endl;
                        dst << reg2 << '\t' << isoCode << Endl;
                        continue;
                    } else if (gbLookup.IsIdInRegion(reg2, reg1)) {
                        ++found;

                        Cerr << "AmbResolved: " << l << " : " << reg2 << " in " << reg1 << Endl;
                        dst << reg1 << '\t' << isoCode << Endl;
                        continue;
                    }
                }

                ++amb;

                Cerr << "Ambigious: " << l << " : scores: ";
                for (const auto& regScore: regIds) {
                    Cerr << " " << regScore.first << "=" << regScore.second;
                }
                Cerr << Endl;
            } else { // == 1
                ++found;

                dst << regIds.cbegin()->first << '\t' << isoCode << Endl;
            }
        }
    }

    Cout << "found\t" << found << Endl
         << "notFound\t" << notFound << Endl
         << "amb\t" << amb << Endl
         << "preset\t" << preset << Endl;

    return 0;
}
