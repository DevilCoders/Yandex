#pragma once

#include <util/stream/output.h>


namespace NAntiRobot {


class TStatsWriter {
public:
    enum class ECounterType {
        Scalar     /* "deee" */,
        Histogram  /* "ahhh" */,
    };

public:
    explicit TStatsWriter(IOutputStream* output);

    ~TStatsWriter();

    void Finish();

    // WithTag and WithPrefix retain a pointer to this.
    TStatsWriter WithTag(TStringBuf key, TStringBuf value);
    TStatsWriter WithPrefix(TString prefix);

    TStatsWriter& Write(TStringBuf key, double value, ECounterType counterType);

    TStatsWriter& WriteScalar(TStringBuf name, double value) {
        return Write(name, value, ECounterType::Scalar);
    }

    TStatsWriter& WriteHistogram(TStringBuf name, double value) {
        return Write(name, value, ECounterType::Histogram);
    }

private:
    explicit TStatsWriter(IOutputStream* output, TStatsWriter* parent, TString prefix);

    void WritePrefix();

private:
    IOutputStream* Output = nullptr;
    TStatsWriter* Parent = nullptr;
    bool ShouldWriteComma = false;
    TString Prefix;
};


} // namespace NAntiRobot
