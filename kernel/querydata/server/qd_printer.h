#pragma once

#include "qd_source.h"

#include <util/memory/blob.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

#include <google/protobuf/text_format.h>

#include <functional>

namespace NQueryData {
    class TFactor;
    class TRawFactor;
    class TSourceFactors;

    struct TPrintOpts {
        bool PrintRaw = false;
        bool PrintAggregated = false;
        bool PrintQueryData = false;

        static TPrintOpts QueryData() {
            TPrintOpts res;
            res.PrintQueryData = true;
            return res;
        }

        static TPrintOpts Raw() {
            TPrintOpts res;
            res.PrintRaw = true;
            return res;
        }
    };

    void Print(TBlob in, IOutputStream& out, const TPrintOpts& opts, const TString& fname = TString());

    using TPrinter = google::protobuf::TextFormat::Printer;

    void InitPrinter(TPrinter&);

    void DumpFactor(IOutputStream& out, TString& buff, const TFactor& f);
    void DumpFactor(IOutputStream& out, TString& buff, TStringBuf name, const TRawFactor& f);

    using TOnSourceFactors = std::function<void(const TSourceFactors&)>;
    TSource::TPtr IterSourceFactors(TBlob trie, TOnSourceFactors onSf);

    TSource::TPtr GetSource(TBlob trie);
}
