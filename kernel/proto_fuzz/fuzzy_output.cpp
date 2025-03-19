#include "fuzzy_output.h"

#include <util/stream/str.h>
#include <util/random/entropy.h>

namespace NProtoFuzz {
    TFuzzyOutput::TFuzzyOutput(IOutputStream& out)
        : TFuzzyOutput(out, TOptions{})
    {}

    TFuzzyOutput::TFuzzyOutput(IOutputStream& out, const TOptions& opts)
        : Out(out)
        , Opts(opts)
        , Rng(0 == opts.Seed ? new TRng(Seed()) : new TRng(opts.Seed))
    {
        Y_ASSERT(Opts.PartSize > 0);
        Opts.PartSize = ::Max<size_t>(1UL, Opts.PartSize);
        PartOffset = Opts.PartSize;
    }

    void TFuzzyOutput::DoWrite(const void* data, size_t len) {
        const char* ptr = static_cast<const char*>(data);
        while (len > 0) {
            if (PartOffset >= Opts.PartSize) {
                PartIndex = Rng->Uniform(Opts.PartSize);
                PartOffset = 0;
            }

            Y_ASSERT(PartOffset < Opts.PartSize);
            const size_t curSize = Min(Opts.PartSize - PartOffset, len);
            const size_t partEnd = PartOffset + curSize;

            if (PartIndex >= PartOffset) {
                const size_t sizeBefore = Min(PartIndex - PartOffset, curSize);
                Out.Write(ptr, sizeBefore);
                ptr += sizeBefore;

                if (PartIndex < partEnd) {
                    const char fuzzyByte = *ptr ^ Rng->Uniform(1, 256);
                    Out.Write(&fuzzyByte, 1);
                    ptr += 1;
                }
            }

            if (PartIndex + 1 < partEnd) {
                const size_t sizeAfter = partEnd - PartIndex - 1;
                Out.Write(ptr, sizeAfter);
                ptr += sizeAfter;
            }

            len -= curSize;
            PartOffset += curSize;
            Y_ASSERT(PartOffset <= Opts.PartSize);
        }
    }

    void FuzzBits(
        TString& data,
        const TFuzzyOutput::TOptions& opts)
    {
        TStringStream out;

        TFuzzyOutput fuzzyOut(out, opts);
        fuzzyOut.Write(data.data(), data.size());

        data = out.Str();
    }
} // NProtoFuzz

