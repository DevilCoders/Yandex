#include <kernel/geo/utils.h>

#include <geobase/include/lookup.hpp>
#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/stream/output.h>

template <typename TestClass>
void TestIsFromParentRegion(TestClass &t, const TString &inputPath)
{
    TFileInput input(inputPath);
    while (true) {
        TGeoRegion reg1;
        TGeoRegion reg2;
        try {
            input >> reg1 >> reg2;
        } catch(...) {
            break; // eof
        }
        Cout << IsFromParentRegion(t, reg1, reg2) << Endl;
    }
}

void PrintDfsVector(const TGeoRegionDFSVector &dfsVector)
{
    Cout << dfsVector.ysize() << Endl;
    for (int i = 0; i < dfsVector.ysize(); i++) {
        Cout << dfsVector[i].first << " " << dfsVector[i].second << Endl;
    }
}

void PrintRelevRegionsDfsVector(const TVector<TGeoRegion> &dfsVector)
{
    Cout << dfsVector.ysize() << Endl;
    for (int i = 0; i < dfsVector.ysize(); i++) {
        Cout << dfsVector[i] << " ";
    }
}

void RunTRegionsDBTest(const TString &geodataPath)
{
    TRegionsDB db(geodataPath);

    const TGeoRegionDFSVector dfsVector = db.GetDFSVector();
    PrintDfsVector(dfsVector);
    Cout << "-----------" << Endl;
    for (int i = 0; i < dfsVector.ysize(); i++) {
        TGeoRegion reg = dfsVector[i].first;
        Cout << db.GetParent(reg) << " " << db.GetName(reg) << " " << (int)db.GetType(reg) << " " << db.GetTimezone(reg) << Endl;
    }
}

void RunIsFromParentRegionTRDBTest(const TString &geodataPath, const TString &inputPath)
{
    TRegionsDB db(geodataPath);

    TestIsFromParentRegion(db, inputPath);
}

void RunTIncExcRegionResolverTest(const TString &geodataPath, const TString &incExcFile, const TString &inputPath)
{
    TRegionsDB db(geodataPath);
    TIncExcRegionResolver resolver(db, incExcFile);
    TFileInput input(inputPath);

    THashSet<TString> labels;
    resolver.GetAllRegLabels(labels);
    THashSet<TString>::iterator it = labels.begin();
    for (; it != labels.end(); ++it) {
        Cout << *it << Endl;
    }
    Cout << "-----------" << Endl;

    TGeoRegion reg;

    while (true) {
        try {
            input >> reg;
        } catch(...) {
            break; // eof
        }
        TVector<TString> labels = resolver.GetRegLabels(reg);
        Cout << labels.ysize() << " > ";
        for (int i = 0; i < labels.ysize(); i++)
            Cout << labels[i] << "\t";
        Cout << Endl;
    }
}

void RunTRelevRegionResolverTest(const TString &geodataPath, const TString &relevRegionsPath, const TString &inputPath)
{
    TRegionsDB db(geodataPath);
    TRelevRegionResolver resolver(geodataPath, relevRegionsPath);

    PrintDfsVector(resolver.GetDFS());
    Cout << "-----------" << Endl;

    TFileInput input(inputPath);
    TGeoRegion reg;

    while (true) {
        try {
            input >> reg;
        } catch(...) {
            break; // eof
        }
        try {
            bool isKnown = resolver.IsKnownRegion(reg);
            Cout << isKnown << Endl;
            Cout << resolver.GetRelevRegion(reg) << Endl;
            if (isKnown) {
                TVector<TGeoRegion> dfsVector;
                resolver.GetRelevRegionsDFS(reg, dfsVector);
                PrintRelevRegionsDfsVector(dfsVector);
            }
        } catch (yexception &e) {
            Cout << "[Exception] " << e.what() << Endl;
        }
    }
    Cout << "~=~=~=~=~=~=~=~" << Endl;
    const TGeoRegionDFSVector& regs = resolver.GetDFS();
    TMap<TGeoRegion, int> testMap;
    for (TGeoRegionDFSVector::const_iterator it = regs.begin(); it != regs.end(); ++it) {
        testMap[it->first] = 0;
    }
    TRelevRegionMapIterator< TMap<TGeoRegion, int>::iterator > testIterator = resolver.GetRRMapIterator(testMap);
    while (testIterator.IsValid()) {
        Cout << testIterator->first << Endl;
        if (testMap.find(testIterator->first) == testMap.end())
            Cout << "Something is bad, returned region " << testIterator->first << " is not found in the original testing map" << Endl;
        TGeoRegion reg = testIterator->first;
        do {
            reg = db.GetParent(reg);
            if (int* count = testMap.FindPtr(reg)) {
                if (*count != 0)
                    Cout << "Wtf, invalid topological sorting" << Endl;
            }
        } while (reg != -1 && reg != 0);
        testMap[testIterator->first]++;
        ++testIterator;
    }
    for (TMap<TGeoRegion, int>::const_iterator it = testMap.begin(); it != testMap.end(); ++it) {
        if (it->second != 1)
            Cout << " Watch out, incorrect testMap value: ";
        Cout << it->first << " / " << it->second << Endl;
    }
}

void RunIsFromParentRegionTRRRTest(const TString &geodataPath, const TString &relevRegionsPath, const TString &inputPath)
{
    TRelevRegionResolver resolver(geodataPath, relevRegionsPath);

    TestIsFromParentRegion(resolver, inputPath);
}

void RunTCheckIsFromYandexTest(const TString &ipregDir, const TString &inputPath)
{
    TCheckIsFromYandex checker(ipregDir);
    TFileInput input(inputPath);

    TString ip;
    while (input.ReadLine(ip)) {
        Cout << checker.IsFromYandex(ip.data());
    }
}

void RunTGeoHelperTest(const TString &ipregDir, const TString &geodataPath, const TString &relevRegionsPath, const TString &inputPath)
{
    TRelevRegionResolver resolver(geodataPath, relevRegionsPath);
    TGeoHelper helper(ipregDir, resolver);

    TFileInput input(inputPath);

    TString ipString;
    while (input.ReadLine(ipString)) {
        try {
            TIp4Or6 ip = Ip4Or6FromString(ipString.data());
            TGeoRegion reg = helper.ResolveRegion(ip);
            TGeoRegion relevReg = helper.GetRelevRegion(reg);
            Cout << reg << "/" << relevReg << " : " << helper.IsFromYandex(ip)
                 << helper.Validate(reg) << helper.ValidateRelev(relevReg) << Endl;
        } catch (yexception &e) {
            Cout << "[Exception] " << e.what() << Endl;
        }
    }
}

void RunTUserTypeResolverTest(const TString &ipregDir, const TString &inputPath)
{
    TUserTypeResolver resolver(ipregDir);
    TFileInput input(inputPath);

    TString ip;
    while (input.ReadLine(ip)) {
        Cout << static_cast<int>(resolver.GetUserType(ip.data()));
    }
}

void RunTIpregRangesTest(const TString &geodataPath, const TString &inputPath)
{
    TIPREG4Or6 ipreg((NGeobase::TLookup(geodataPath)));
    TFileInput input(inputPath);

    TString inputLine;
    while (input.ReadLine(inputLine)) {
        Cout << ipreg.ResolveRegion(Ip4Or6FromString(inputLine.data())) << "\n";
    }
}

int main(int argc, const char* argv[])
{
    TString geodataPath;
    TString ipregDir;
    TString ipregRanges;
    TString relevRegionsPath;
    TString isFromParentRegionTRDB;
    TString isFromParentRegionTRRR;
    TString incExcRegionResolver;
    TString incExcRegionsPath;
    TString relevRegionResolver;
    TString checkIsFromYandex;
    TString geoHelper;
    TString userTypeResolver;

    NLastGetopt::TOpts opts;
    opts
        .AddLongOption("geodata", "")
        .Required()
        .StoreResult(&geodataPath);
    opts
        .AddLongOption("ipreg", "")
        .Optional()
        .StoreResult(&ipregDir);
    opts
        .AddLongOption("relev_reg", "")
        .Optional()
        .StoreResult(&relevRegionsPath);
    opts
        .AddLongOption("TIpregRanges", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&ipregRanges);
    opts
        .AddLongOption("TRegionsDB", "")
        .Optional()
        .NoArgument();
    opts
        .AddLongOption("IsFromParentRegionTRDB", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&isFromParentRegionTRDB);
    opts
        .AddLongOption("TIncExcRegionResolver", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&incExcRegionResolver);
    opts
        .AddLongOption("incexc_regions", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&incExcRegionsPath);
    opts
        .AddLongOption("TRelevRegionResolver", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&relevRegionResolver);
    opts
        .AddLongOption("IsFromParentRegionTRRR", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&isFromParentRegionTRRR);
    opts
        .AddLongOption("TCheckIsFromYandex", "")
        .Optional()
        .DefaultValue("")
        .StoreResult(&checkIsFromYandex);
    opts
        .AddLongOption("TGeoHelper")
        .Optional()
        .DefaultValue("")
        .StoreResult(&geoHelper);
    opts
        .AddLongOption("TUserTypeResolver")
        .Optional()
        .DefaultValue("")
        .StoreResult(&userTypeResolver);
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);

    if (optsRes.Has("TRegionsDB")) {
        RunTRegionsDBTest(geodataPath);
    }

    if (ipregRanges) {
        RunTIpregRangesTest(geodataPath, ipregRanges);
    }

    if (isFromParentRegionTRDB) {
        RunIsFromParentRegionTRDBTest(geodataPath, isFromParentRegionTRDB);
    }

    if (incExcRegionResolver && incExcRegionsPath) {
        RunTIncExcRegionResolverTest(geodataPath, incExcRegionsPath, incExcRegionResolver);
    }

    if (relevRegionResolver) {
        RunTRelevRegionResolverTest(geodataPath, relevRegionsPath, relevRegionResolver);
    }

    if (isFromParentRegionTRRR) {
        RunIsFromParentRegionTRRRTest(geodataPath, relevRegionsPath, isFromParentRegionTRRR);
    }

    if (checkIsFromYandex) {
        RunTCheckIsFromYandexTest(ipregDir, checkIsFromYandex);
    }

    if (geoHelper) {
        RunTGeoHelperTest(ipregDir, geodataPath, relevRegionsPath, geoHelper);
    }

    if (userTypeResolver) {
        RunTUserTypeResolverTest(ipregDir, userTypeResolver);
    }

    return 0;
}
