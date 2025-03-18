#include "stats_writer.h"


namespace NAntiRobot {


TStatsWriter::TStatsWriter(IOutputStream* output)
    : Output(output)
    , Prefix("[\"")
{
    *Output << "[";
}

TStatsWriter::~TStatsWriter() {
    try {
        Finish();
    } catch (...) {}
}

void TStatsWriter::Finish() {
    if (Output && !Parent) {
        *Output << ']';
        Output = nullptr;
    }
}

TStatsWriter TStatsWriter::WithTag(TStringBuf key, TStringBuf value) {
    return TStatsWriter(Output, this, TString::Join(key, "=", value, ";"));
}

TStatsWriter TStatsWriter::WithPrefix(TString prefix) {
    return TStatsWriter(Output, this, std::move(prefix));
}

TStatsWriter& TStatsWriter::Write(TStringBuf key, double value, ECounterType counterType) {
    if (Output) {
        WritePrefix();
        *Output << key << "_" << counterType << "\"," << value << "]";
    }

    return *this;
}

TStatsWriter::TStatsWriter(IOutputStream* output, TStatsWriter* parent, TString prefix)
    : Output(output)
    , Parent(parent)
    , Prefix(std::move(prefix))
{}

void TStatsWriter::WritePrefix() {
    if (Parent) {
        Parent->WritePrefix();
    } else {
        if (ShouldWriteComma) {
            *Output << ',';
        } else {
            ShouldWriteComma = true;
        }
    }

    *Output << Prefix;
}


} // namespace NAntiRobot
