#include <util/stream/output.h>

#include <kernel/geo/utils.h>
#include <library/cpp/deprecated/prog_options/prog_options.h>

int main(int argc, const char** argv)
{
    TProgramOptions progOptions("|geodata|+|relev_regs|+|e|+|ipreg|+|relev_reg|+|reg|+|from_y|+|relev_dfs|+|dfs|+");
    progOptions.Init(argc, argv);

    TString envDir = progOptions.GetOptVal("e","./");

    bool act = false;

    TOptRes regOpt = progOptions.GetOption("reg");
    if (regOpt.first) {
        TString ipregDir = progOptions.GetDirOptVal("ipreg", envDir + LOCSLASH_S "ipreg");
        TIPREG4Or6 ipreg(ipregDir);

        Cout << ipreg.ResolveRegion(Ip4Or6FromString(regOpt.second)) << Endl;
        act = true;
    }

    TOptRes rrOpt = progOptions.GetOption("relev_reg");
    if (!act && rrOpt.first) {
        TString geodataPath = progOptions.GetOptVal("geodata", envDir + "geodata3.bin");
        TString relevRegs = progOptions.GetOptVal("relev_regs", envDir + "relev_regions.txt");
        TRelevRegionResolver resolver(geodataPath, relevRegs, -1);
        Cout << resolver.GetRelevRegion(FromString<TGeoRegion>(rrOpt.second)) << Endl;
        act = true;
    }

    TOptRes dfsOpt = progOptions.GetOption("dfs");
    if (!act && dfsOpt.first) {
        TRegionsDB regionsDB(progOptions.GetOptVal("geodata", envDir + "geodata3.bin"));

        TGeoRegion reg = FromString<TGeoRegion>(dfsOpt.second);
        while (reg != -1) {
            Cout << ' ' << reg;
            reg = regionsDB.GetParent(reg);
        }
        Cout << Endl;
        act = true;
    }

    TOptRes rdOpt = progOptions.GetOption("relev_dfs");
    if (!act && rdOpt.first) {
        TString geodataPath = progOptions.GetOptVal("geodata", envDir + "geodata3.bin");
        TString relevRegs = progOptions.GetOptVal("relev_regs", envDir + "relev_regions.txt");
        TRelevRegionResolver resolver(geodataPath, relevRegs, -1);
        TVector<TGeoRegion> dfs;
        resolver.GetRelevRegionsDFS(FromString<TGeoRegion>(rdOpt.second), dfs);
        for (TVector<TGeoRegion>::const_iterator it = dfs.begin(); it != dfs.end(); ++it)
            Cout << ' ' << *it;
        Cout << Endl;
        act = true;
    }

    TOptRes fyOpt = progOptions.GetOption("from_y");
    if (!act && fyOpt.first) {
        TString ipregDir = progOptions.GetDirOptVal("ipreg", envDir + LOCSLASH_S "ipreg");
        TCheckIsFromYandex yipchecker(ipregDir.data());
        Cout << yipchecker.IsFromYandex(fyOpt.second) << Endl;
        act = true;
    }

    if (!act) {
        ythrow yexception() << "No action";
    }
    return 0;
}

