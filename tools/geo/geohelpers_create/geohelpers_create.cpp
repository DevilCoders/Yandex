
#include <kernel/geo/utils.h>

#include <library/cpp/binsaver/util_stream_io.h>

#include <library/cpp/deprecated/prog_options/prog_options.h>


const char* optSpecString = "|geodata|+|ipreg|+|relev_reg|+|dst_geohelper|+|dst_utr|+";


void PrintHelp()
{
    Cout << "Usage: geohelpers_create <options>\n"
            " -ipreg <path>         - path to ipreg dir\n"
            "[-relev_reg <path>]    - path to relev_regions files\n"
            "                         (if omitted relevRegionResolver will be disfunctional in the result file)\n"
            "[-geodata <path>]      - path to geodata3.bin (needed only if relev_reg is specified)\n"
            " -dst_geohelper <path> - path to dst geohelper file\n"
            " -dst_utr <path>       - path to dst user_type_resolver file\n";
}


int GeohelpersCreate(TProgramOptions& progOptions)
{
    TString dstGeohelper = progOptions.GetReqOptVal("dst_geohelper");
    TString dstUTR = progOptions.GetReqOptVal("dst_utr");

    TString ipregDir = progOptions.GetReqDirOptVal("ipreg");

    TOptRes rrOpt = progOptions.GetOption("relev_reg");

    THolder<TRelevRegionResolver> rrr(
            rrOpt.first ? new TRelevRegionResolver(progOptions.GetReqOptVal("geodata"),
                                                   rrOpt.second)
                        : new TRelevRegionResolver
        );

    TGeoHelper geoHelper(ipregDir, *rrr);
    SerializeToFile(dstGeohelper, geoHelper);

    TUserTypeResolver utr(ipregDir);
    SerializeToFile(dstUTR, utr);

    return 0;
}

int main(int argc, const char *argv[])
{
    TProgramOptions progOptions(optSpecString);
    return main_with_options_and_catch(progOptions, argc, argv, GeohelpersCreate, PrintHelp);
}
