#pragma once

#include <util/stream/input.h>
#include <util/stream/output.h>

class TransformTables {
    char transformFromMRTable[256];
    char transformToMRTable[256];

    void TransformString(const char(&table)[256], char* str, size_t length) const;
    void TransformString(const char(&table)[256], TString str) const;

public:
    TransformTables();
    void TransformFromMR(TString str) const;
    void TransformToMR(TString str) const;
};

class SingleLineMapreduceFormatProcessor {
    static TransformTables Transforms;

public:
    static void ProcessHtml(IInputStream& input, IOutputStream& output);
};
