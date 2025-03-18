#include "stats_output.h"

#include <util/system/compiler.h>

#include <initializer_list>

namespace {

class TStatsOutputDecorator : public TStatsOutput {
public:
    TStatsOutputDecorator(TStatsOutput& slave)
        : Slave(slave)
    {
    }

    THolder<TStatsOutput> StartGroup(const TString& name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params, char delimiter) override {
        return Slave.StartGroup(name, params, delimiter);
    }

    TStatsOutput& AddValue(const TString& name, double value, EMetric suffix) override {
        return Slave.AddValue(name, value, suffix);
    }

    IOutputStream& GetStream() override {
        return Slave.GetStream();
    }

protected:
    TStatsOutput& Slave;
};

class TJsonGroup : public TStatsOutputDecorator {
public:
    TJsonGroup(TStatsOutput& slave, TString name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params = {}, char delimiter = '.')
        : TStatsOutputDecorator(slave)
        , Prefix(std::move(name))
        , Delimiter(delimiter)
    {
        for (const auto& keyValue : params) {
            Prefix += TString::Join("_", keyValue.first, "_", keyValue.second);
        }
    }

    TStatsOutput& AddValue(const TString& name, double value, EMetric suffix) override {
        Slave.AddValue(TString::Join(Prefix, Delimiter, name), value, suffix);
        return *this;
    }

    THolder<TStatsOutput>
    StartGroup(const TString &name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params, char delimiter) override {
        return MakeHolder<TJsonGroup>(*this, name, params, delimiter);
    }

private:
    TString Prefix;
    char Delimiter;
};

}

THolder<TStatsOutput> TStatsJsonOutput::StartGroup(const TString& name, std::initializer_list<std::pair<TStringBuf, TStringBuf>> params, char delimiter) {
    return MakeHolder<TJsonGroup>(*this, name, params, delimiter);
}

TStatsOutput& TStatsJsonOutput::AddValue(const TString& name, double value, EMetric suffix) {
    if (Y_LIKELY(!FirstValue)) {
        Stream << ',';
    }
    FirstValue = false;

    Stream << "[\"" << name << "_" << suffix << "\", " << value << "]";
    return *this;
}
