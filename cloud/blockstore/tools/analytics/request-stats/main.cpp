#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/linear_regression/linear_regression.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <util/stream/input.h>
#include <util/string/cast.h>

namespace {

using namespace NLastGetopt;

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString InputFile;

    void Parse(int argc, char** argv)
    {
        TOpts opts;
        opts.AddHelpOption();

        opts.AddLongOption("input", "input file name")
            .RequiredArgument()
            .StoreResult(&InputFile);

        TOptsParseResultException res(&opts, argc, argv);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTraceRecord
{
    TString RequestId;
    TString RequestType;
    TString RequestSize;
    TString RequestTime;
};

////////////////////////////////////////////////////////////////////////////////

struct ITraceVisitor
{
    virtual ~ITraceVisitor() = default;

    virtual void OnRecord(const TTraceRecord& record)
    {
        Y_UNUSED(record);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TTraceParser final
    : public NJson::TJsonCallbacks
{
private:
    ITraceVisitor& Visitor;

    TTraceRecord Record;
    TString* Value = nullptr;

public:
    TTraceParser(ITraceVisitor& visitor)
        : TJsonCallbacks(true)  // throwException
        , Visitor(visitor)
    {}

    bool Parse(IInputStream& in)
    {
        return NJson::ReadJson(&in, this);
    }

private:
    bool OnMapKey(const TStringBuf& s) override
    {
        if (s == "requestId"sv) {
            Value = &Record.RequestId;
        } else if (s == "requestType"sv) {
            Value = &Record.RequestType;
        } else if (s == "requestSize"sv) {
            Value = &Record.RequestSize;
        } else if (s == "requestTime"sv) {
            Value = &Record.RequestTime;
        } else {
            Value = nullptr;
        }
        return true;
    }

    bool OnString(const TStringBuf& s) override
    {
        if (Value) {
            Value->assign(s);
        }
        return true;
    }

    bool OnCloseArray() override
    {
        if (Record.RequestType) {
            Visitor.OnRecord(Record);
        }
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TTraceAnalyzer final
    : public ITraceVisitor
{
    using TSample = std::pair<size_t, double>;
    using TSamples = TVector<TSample>;

    struct TModel
    {
        double Factor;
        double FixedCost;

        double operator ()(size_t size) const
        {
            return FixedCost + Factor * size;
        }
    };

private:
    TMap<TString, TSamples> RequestSamples;

public:
    void Finish()
    {
        for (auto& kv: RequestSamples) {
            auto model = CalcModel(kv.second);
            model = CalcModel(RemoveOutliers(kv.second, model, 0.9));

            auto bandwidth = model.Factor > 0 ? 1000000. / model.Factor : 0;
            Cout << "Request: " << kv.first
                 << ", Samples: " << kv.second.size()
                 << ", Bandwidth: " << bandwidth << " blocks/s"
                 << ", RTT: " << TDuration::MicroSeconds(model.FixedCost)
                 << Endl;
        }
    }

private:
    void OnRecord(const TTraceRecord& record) override
    {
        ui64 requestSize = FromString<ui64>(record.RequestSize);
        ui64 requestTime = FromString<ui64>(record.RequestTime);

        if (requestSize && requestTime) {
            RequestSamples[record.RequestType].emplace_back(requestSize, requestTime);
        }
    }

    TModel CalcModel(const TSamples& samples)
    {
        TKahanSLRSolver solver;
        for (const auto& s: samples) {
            solver.Add(s.first, s.second);
        }

        double factor = 0.;
        double fixedCost = 0.;
        solver.Solve(factor, fixedCost);

        return { factor, fixedCost };
    }

    TSamples RemoveOutliers(const TSamples& samples, const TModel& model, double fraction)
    {
        using TSampleWithError = std::pair<const TSample*, double>;

        TVector<TSampleWithError> ss(Reserve(samples.size()));
        for (const auto& s: samples) {
            ss.emplace_back(&s, fabs((model(s.first) - s.second)));
        }

        Sort(ss.begin(), ss.end(),
            [] (const TSampleWithError& l, const TSampleWithError& r) {
                return (l.second < r.second)
                    || ((l.second == r.second) && (l.first < r.first));
            });

        TSamples result(Reserve(samples.size()));
        for (const auto i: xrange<size_t>(0, fraction * ss.size())) {
            result.push_back(*ss[i].first);
        }
        return result;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    TOptions opts;
    opts.Parse(argc, argv);

    auto input = OpenInput(opts.InputFile);

    TTraceAnalyzer analyzer;

    TTraceParser parser(analyzer);
    parser.Parse(*input);

    analyzer.Finish();

    return 0;
}
