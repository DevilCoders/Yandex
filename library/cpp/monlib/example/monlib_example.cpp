#include <util/stream/file.h>
#include <util/generic/cast.h>
#include <util/random/random.h>

#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/service/pages/pre_mon_page.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>
#include <library/cpp/monlib/service/pages/diag_mon_page.h>
#include <library/cpp/monlib/service/pages/templates.h>
#include <library/cpp/monlib/service/auth/tvm/auth.h>
#include <library/cpp/monlib/service/pages/registry_mon_page.h>

#include <library/cpp/monlib/metrics/metric.h>
#include <library/cpp/monlib/metrics/metric_registry.h>

#include <library/cpp/getopt/last_getopt.h>


using namespace NMonitoring;

struct TDateTimePage: public TPreMonPage {
    TDateTimePage()
        : TPreMonPage("date-time", "Date and Time")
    {
    }

    void OutputText(IOutputStream& os, NMonitoring::IMonHttpRequest&) override {
        os << "current time is " << TInstant::Now();
    }
};

struct TTextFilePage: public TPreMonPage {
    const TString FileName;

    TTextFilePage(const TString& path, const TString& title, const TString& fileName)
        : TPreMonPage(path, title)
        , FileName(fileName)
    {
    }

    void OutputText(IOutputStream& os, NMonitoring::IMonHttpRequest&) override {
        TUnbufferedFileInput in(FileName);
        os << in.ReadAll();
    }
};

struct TTemplateEnginePageExample: public TPreMonPage {
    TTemplateEnginePageExample()
        : TPreMonPage("template-engine", "Template Engine Example", false)
    {
        Table.push_back(std::pair<TString, TString>("foo1", "bar1"));
        Table.push_back(std::pair<TString, TString>("foo2", "bar2"));
        Table.push_back(std::pair<TString, TString>("foo3", "bar3"));
        Table.push_back(std::pair<TString, TString>("foo4", "bar4"));
        Table.push_back(std::pair<TString, TString>("foo5", "bar5"));
    }

    void OutputText(IOutputStream& os, NMonitoring::IMonHttpRequest&) override {
        // output a table according to bootstrap grid settings
        HTML(os) {
            DIV_CLASS("row") {
                DIV_CLASS("col-md-4") {
                }
                DIV_CLASS("col-md-4") {
                    OutputTable(os);
                }
                DIV_CLASS("col-md-4") {
                }
            }
        }
    }

    void OutputTable(IOutputStream& os) {
        // output a table using HTML templates
        HTML(os) {
            TABLE_CLASS("table table-condensed") {
                CAPTION() {
                    os << "Foo-Bar Table";
                }
                TABLEHEAD() {
                    TABLER() {
                        TABLEH() {
                            os << "Foo";
                        }
                        TABLEH() {
                            os << "Bar";
                        }
                    }
                }
                TABLEBODY() {
                    for (unsigned i = 0; i < Table.size(); i++) {
                        TABLER() {
                            TABLED() {
                                os << Table[i].first;
                            }
                            TABLED() {
                                os << Table[i].second;
                            }
                        }
                    }
                }
            }
        }
    }

private:
    TVector<std::pair<TString, TString>> Table;
};

int main(int argc, char** argv) {
    using namespace NLastGetopt;

    ui16 port;
    TVector<NTvmAuth::TTvmId> clientIds;
    NTvmAuth::TTvmId selfId{0};

    TOpts opts;
    opts.AddLongOption('p', "port")
        .RequiredArgument("PORT")
        .StoreResult(&port)
        .DefaultValue(8901)
        .Optional();

    opts.AddLongOption("tvm-dst-id")
        .RequiredArgument("CLIENT_ID")
        .AppendTo(&clientIds)
        .Help("Destination service ids. May be specified multiple times")
        .Optional();

    opts.AddLongOption("tvm-src-id")
        .RequiredArgument("CLIENT_ID")
        .StoreResult(&selfId)
        .Optional();

    opts.SetFreeArgsNum(0);
    TOptsParseResult res(&opts, argc, argv);

    auto authType = selfId || !clientIds.empty()
        ? EAuthType::Tvm
        : EAuthType::None;

    THolder<IAuthProvider> auth;
    switch (authType) {
        case EAuthType::None:
            auth = CreateFakeAuth();
            break;;
        case EAuthType::Tvm: {
            Y_ENSURE(selfId && !clientIds.empty(), "Both --tvm-dst-id and --tvm-src-id must be specified in order to use TVM");
            NTvmAuth::NTvmApi::TClientSettings settings;
            settings.SetSelfTvmId(selfId);
            settings.EnableServiceTicketChecking();

            auto manager = CreateDefaultTvmManager(std::move(settings), clientIds);
            auth = CreateTvmAuth(std::move(manager));
            break;
        }
    };

    TMonService2 monService(port, "Monlib Example", std::move(auth));

    TMetricRegistry registry({{"common", "label"}});
    TCounter* countersPageRequested = registry.Counter({{"sensor", "countersPageRequested"}});
    countersPageRequested->Inc();
    TCounter* random = registry.Counter({{"sensor", "random"}});
    random->Add(RandomNumber(10u));

    TCounter* firstSlash = registry.Counter({{"sensor", "first"}, {"subsystem", "blabla/slash"}});
    firstSlash->Add(RandomNumber(100u));
    TCounter* secondSlash = registry.Counter({{"sensor", "second"}, {"subsystem", "blabla/slash"}});
    secondSlash->Add(RandomNumber(200u));
    TCounter* firstSpace = registry.Counter({{"sensor", "first"}, {"subsystem", "blabla space"}});
    firstSpace->Add(RandomNumber(100u));
    TCounter* zzzzFoobar = registry.Counter({{"sensor", "zzzz"}, {"subsystem", "foobar"}});
    zzzzFoobar->Add(RandomNumber(4000u));

    TCounter* privateFoo = registry.Counter({{"sensor", "foo"}, {"subsystem", "private"}});
    privateFoo->Add(RandomNumber(5u));

    TCounter* privateFoobar = registry.Counter({{"sensor", "private"}, {"subsystem", "foobar"}});
    privateFoobar->Add(RandomNumber(4000u));

    TVector<TCounter*> percentiles;
    percentiles.push_back(registry.Counter({{"sensor", "histogram"}, {"percentile", "25.0"}}));
    percentiles.push_back(registry.Counter({{"sensor", "histogram"}, {"percentile", "50.0"}}));
    percentiles.push_back(registry.Counter({{"sensor", "histogram"}, {"percentile", "75.0"}}));
    percentiles.push_back(registry.Counter({{"sensor", "histogram"}, {"percentile", "100.0"}}));
    for (auto percentile : percentiles) {
        percentile->Add(RandomNumber(100u));
    }

    // register services
    monService.Register(new TDateTimePage);
    monService.Register(new TVersionMonPage);
    monService.Register(new TDiagMonPage);
    monService.Register(new TMetricRegistryPage("metrics", "Metrics", &registry));
    monService.Register(new TTemplateEnginePageExample());

    monService.RegisterIndexPage("proc", "System Info");

    monService.FindIndexPage("proc")
        ->Register(new TTextFilePage("cpuinfo", "CPU", "/proc/cpuinfo"));
    monService.FindIndexPage("proc")
        ->Register(new TTextFilePage("uptime", "Uptime", "/proc/uptime"));

    try {
        monService.StartOrThrow();

        Cerr << "started mon service on port http://localhost:" << port << "\n";

        for (;;) {
            Sleep(TDuration::Hours(1));
        }

        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
