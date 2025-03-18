#pragma once

#include <util/stream/input.h>
#include <util/stream/str.h>

void ReadOneTestChunk(IInputStream* input, TString* test) {
    TStringStream dataStream;
    TStringStream resultStream;

    bool readData = false;
    bool readErrs = false;
    bool readDocument = false;

    for (TString line = input->ReadLine(); true;
         line = input->ReadLine()) {
        if (line == "#data") {
            readData = true;
            continue;
        }

        if (line == "#errors") {
            readData = false;
            readErrs = true;
            continue;
        }

        if (line == "#document" || line == "#document-fragment") {
            readErrs = false;
            readDocument = true;
            continue;
        }

        if (readData) {
            if (line.empty()) {
                dataStream << Endl;
            } else {
                dataStream << line;
            }
        }

        if (readErrs) {
            continue;
        }

        if (readDocument) {
            if (line.empty())
                break;
            resultStream << line << Endl;
        }
    }

    *test = dataStream.Str();
}
