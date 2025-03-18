#pragma once

#include "randoms.h"
#include "version.h"

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

class TInnerProductSimhash: private TNonCopyable {
public:
    typedef TRandomVectors::TRandomValue TInValue;
    typedef bool TOutValue;
    typedef TVector<TInValue> TIn;
    typedef TVector<TOutValue> TOut;

    static ui32 GetVersion() {
        return 1;
    }
    static TString GetDescription() {
        return "vector inner product based simhash";
    }

public:
    TInnerProductSimhash(
        const TSimHashVersion& version,
        TIn* Input,
        TOut* Output)
        : Version(version)
        , input(Input)
        , output(Output)
        , vectors(RandomVectorsFactory().GetRandomVectors(Version.RandomVersion))
    {
        Y_VERIFY((input != nullptr), "input == NULL");
        Y_VERIFY((output != nullptr), "output == NULL");
    }

public:
    void ClearInput() {
        input->clear();
        input->resize((size_t)vectors.Length(), 0);
    }

    void ClearOutput() {
        output->clear();
        output->resize((size_t)vectors.Count(), false);
    }

    void Calculate() {
        Y_VERIFY(((ui32)input->size() == vectors.Length()), "Wrong input size");
        Y_VERIFY(((ui32)output->size() == vectors.Count()), "Wrong output size");
        const ui32 count = vectors.Count();
        for (ui32 i = 0; i < count; ++i) {
            if (InnerProduct(i) >= 0) {
                output->operator[](i) = true;
            }
        }
    }

    ui32 GetBitCount() const {
        return vectors.Count();
    }

private:
    TInValue InnerProduct(ui32 I) const {
        TInValue res = 0;
        const ui32 length = vectors.Length();
        for (ui32 i = 0; i < length; ++i) {
            res += input->operator[](i) * vectors.Vectors()[I][i];
        }
        return res;
    }

private:
    const TSimHashVersion& Version;
    TIn* input;
    TOut* output;
    const TRandomVectors& vectors;
};
