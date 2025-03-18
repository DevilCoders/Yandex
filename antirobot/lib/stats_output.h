#pragma once

#include <util/stream/str.h>
#include <util/string/cast.h>

#include <initializer_list>

enum class EMetric {
    Scalar    /* "deee" */,
    Histogram /* "ahhh" */,
};

class TStatsOutput {
public:
    virtual ~TStatsOutput() { }

    virtual THolder<TStatsOutput>
    StartGroup(const TString& name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params = {}, char delimiter = '.') = 0;
    virtual TStatsOutput& AddValue(const TString& name, double value, EMetric suffix) = 0;
    virtual IOutputStream& GetStream() = 0;

    TStatsOutput& AddScalar(const TString& name, double value) {
        return AddValue(name, value, EMetric::Scalar);
    }

    TStatsOutput& AddHistogram(const TString& name, double value) {
        return AddValue(name, value, EMetric::Histogram);
    }
};

class TStatsJsonOutput: public TStatsOutput {
public:
    TStatsJsonOutput(IOutputStream& os)
        : Stream(os)
    {
        Stream << '[';
    }

    ~TStatsJsonOutput() {
        Stream << ']';
    }

    THolder<TStatsOutput> StartGroup(const TString& name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params = {}, char delimiter = '.') override;
    TStatsOutput& AddValue(const TString& name, double value, EMetric suffix) override;
    IOutputStream& GetStream() override {
        return Stream;
    }

private:
    bool FirstValue = true;
    IOutputStream& Stream;
};
