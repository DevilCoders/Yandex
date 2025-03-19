#pragma once

#include <library/cpp/deprecated/calc_module/misc_points.h>
#include <library/cpp/deprecated/calc_module/simple_module.h>

#include <yweb/protos/robot.pb.h>
#include <yweb/protos/hostfactors.pb.h>
#include <yweb/robot/genref/genref_lib/cattygeo_parser.h>

#include <library/cpp/string_utils/url/url.h> // for GetHost and CutHttpPrefix

// For catty and geo retrieveing.
// use TCattyAndGeoParser, CatFilter and C2P, HostFactors
class TCattyAndGeoParserModule : public TSimpleModule {
private:
    typedef ::TAnchorText_TExtraHostFactors TExtraHostFactors;

    class TExtraHostFactorsWriter {
    private:
        TExtraHostFactors* ExtraFactors;
    public:
        TExtraHostFactorsWriter(TExtraHostFactors* extraFactors)
            : ExtraFactors(extraFactors)
        {}

        void WriteGeo(i32 geo) {
            ExtraFactors->SetOwnerGeo(geo);
        }

        void WriteCatty(ui32 catty) {
            ExtraFactors->AddOwnerCatty(catty);
        }
    };

    typedef THashMap<ui32, ui32> TC2PStore;

    THolder<TCattyAndGeoParser> CattyAndGeoParser;

    TMasterCopyPoint<const TCatFilterHolder::TICatFilterHolder*> CatFilterInputPoint;
    TMasterCopyPoint<const TC2PStore*> C2PInputPoint;
    TMaster2ArgsPoint<const TString&, NRealTime::THostFactors&> HostFactorsInputPoint;
    TMasterAnswerPoint<const TString&, TString> MirrorOwnerInputPoint;
    const bool UseHostFactors;

public:
    TCattyAndGeoParserModule(bool useHostFactors = true)
        : TSimpleModule("TCattyAndGeoParserModule")
        , CatFilterInputPoint(this, /*default=*/nullptr, "catfilter_input")
        , C2PInputPoint(this, /*default=*/nullptr, "c2p_input")
        , MirrorOwnerInputPoint(this, "mirror_owner_input")
        , UseHostFactors(useHostFactors)
    {
        if (UseHostFactors) {
            AddAccessPoint("host_factors_input", &HostFactorsInputPoint);
        }

        Bind(this).To<&TCattyAndGeoParserModule::Init>("init");
        Bind(this).To<const TString&, TExtraHostFactors&, &TCattyAndGeoParserModule::GetExtraFactors>("extra_factors_output");
        AddInitDependency("catfilter_input");
        AddInitDependency("c2p_input");
    }

private:
    void Init() {
        CattyAndGeoParser.Reset(new TCattyAndGeoParser(*CatFilterInputPoint.GetValue()->Get(), *C2PInputPoint.GetValue()));
    }

    void GetExtraFactors(const TString& url, TExtraHostFactors& extraFactors) {
        // Assume host are normalized and mirror resolved.
        TString host = TString{GetHost(CutHttpPrefix(to_lower(url)))};
        NRealTime::THostFactors hostFactors;

        if (UseHostFactors) {
            // works on;y in rtindexer
            HostFactorsInputPoint.Func(host, hostFactors);
            extraFactors.SetOwnerTic(hostFactors.GetTICReal()); // !!set real TIC!!
        }

        TString ownerName  = MirrorOwnerInputPoint.Answer(host); //FactorsReader->GetMirrorOwner(host, /*getReversed=*/false);
        TExtraHostFactorsWriter extraFactorsWriter(&extraFactors);
        CattyAndGeoParser->ParseCattyAndGeo(extraFactorsWriter, ownerName);
    }
};
