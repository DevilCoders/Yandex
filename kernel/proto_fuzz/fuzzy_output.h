#pragma once

#include <util/random/fast.h>
#include <util/stream/output.h>
#include <util/generic/ptr.h>

namespace NProtoFuzz {
    // Fuzzes data stream before
    // writing it to Out.
    // Each chunk of PartSize length
    // has exactly 1 modified byte.
    //
    // TODO: deletions, insertions, random suffix
    //
    class TFuzzyOutput
        : public IOutputStream
    {
    public:
        struct TOptions {
            size_t PartSize = 100;
            ui64 Seed = 0;

            TOptions() = default;
            explicit TOptions(ui64 seed)
                : Seed(seed)
            {}

            TOptions ReSeed(ui64 seed) const {
                TOptions res = *this;
                res.Seed = seed;
                return res;
            }
        };

        using TRng = TFastRng64;

    public:
        TFuzzyOutput(IOutputStream& out);
        TFuzzyOutput(IOutputStream& out, const TOptions& opts);

    protected:
        void DoWrite(const void* buf, size_t len) override;

    private:
        IOutputStream& Out;
        TOptions Opts;
        THolder<TRng> Rng;
        size_t PartIndex = 0;
        size_t PartOffset = 0;
    };

    void FuzzBits(
        TString& data,
        const TFuzzyOutput::TOptions& opts = {});
} // NProtoFuzz
