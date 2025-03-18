#include <tools/snipmake/snipdat/metahost.h>
#include <tools/snipmake/snipdat/xmlsearchin.h>
#include <tools/snipmake/snipmetrics/metriclist.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

static const TString ExtraCgiParams = "gta=_UrlMenu";
static const TString GroupBy = "attr=d.mode=deep.groups-on-page=30.docs-in-group=1";
static const TString Source = "WEB";
static const TString Type = "";

namespace NSnippets {
    static size_t GetColoredWordsCount(const TString& text) {
        static const char marker[] = "\x07[";
        size_t cnt = 0;
        size_t pos = 0;
        while (pos != TString::npos) {
            pos = text.find(marker, pos);
            if (pos != TString::npos) {
                ++cnt;
                ++pos;
            }
        }
        return cnt;
    }

    struct TUrlmenuStatInfo {
        bool Has;
        bool Showed;
        TUrlmenuStatInfo(const NXmlSearchIn::TDocument& doc)
            : Has(false)
            , Showed(false)
        {
            if (!doc.SerializedUrlmenu)
                return;

            TStringInput in(doc.SerializedUrlmenu);
            NJson::TJsonValue urlmenu;
            if (!NJson::ReadJsonTree(&in, &urlmenu))
                return;
            size_t urlmenuCount = 0;
            for (const NJson::TJsonValue& item : urlmenu.GetArraySafe()) {
                if (item.GetArraySafe().size() < 2)
                    return;
                const TString& name = item.GetArraySafe()[1].GetStringSafe();
                urlmenuCount += GetColoredWordsCount(name);
            }
            Has = true;
            Showed = urlmenuCount >= GetColoredWordsCount(doc.HilitedUrl);
        }
    };

    class TGtaMetricsCalculator {
    private:
        TMetricArray Stat;

    public:
        void operator() (const NXmlSearchIn::TDocument& doc) {
            const TUrlmenuStatInfo urlmenuInfo(doc);
            ProcessMetricValue(MN_HAS_URLMENU, urlmenuInfo.Has);
            ProcessMetricValue(MN_SHOWED_URLMENU, urlmenuInfo.Showed);
        }

        void Report() {
            Stat.WriteAsXml("Gta metrics", &Cout);
        }

    private:
        void ProcessMetricValue(EMetricName metricName, double value) {
            Stat.SumValue(metricName, value);
        }

    };
}

int main(int argc, char** argv) {
    using namespace NLastGetopt;

    TString inFile;

    TOpts opt;
    opt.AddLongOption('i', "input", "input file, stdin is used if omitted").StoreResult(&inFile);
    opt.SetFreeArgsMax(0);
    TOptsParseResult o(&opt, argc, argv);

    IInputStream* input = inFile.size() ? new TFileInput(inFile) : &Cin;

    NSnippets::TDomHosts hosts("", ExtraCgiParams);
    NSnippets::NXmlSearchIn::TRequest res;
    TString line;
    NSnippets::TGtaMetricsCalculator calculator;
    while (input->ReadLine(line)) {
        TVector<TString> v;
        Split(line.data(), "\t", v);
        if (v.size() < 3) {
            continue;
        }
        TString req = v[0];
        TString region = v[1];
        TString domRegion = v[2];
        const NSnippets::THost& dhost = hosts.Get(domRegion);
        try {
            if (!dhost.XmlSearch(res, req, Source, Type, region, GroupBy)) {
                continue;
            }
            for (const auto& doc : res.Documents) {
                calculator(doc);
            }
        } catch (...) {
        }
    }
    calculator.Report();
    return 0;
}
