#pragma once

#include <kernel/segmentator/structs/structs.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/url/url.h>

#include <limits>

namespace NSegm {
namespace NPrivate {

struct TFeatureArray {
    ui32 Number = 0;
    TString Url;
    float Relev = 0;
    ui32 Lcs = 0;
    ui32 RealContentSize = 0;
    ui32 ContentSize = 0;
    float Weight = 0;
    TVector<float>& F;

    explicit TFeatureArray(ui32 num, ui32 fsize, TVector<float>& features)
        : Number(num)
        , Relev()
        , F(features)
    {
        features.resize(fsize);
    }

    friend bool operator<(const TFeatureArray& a, const TFeatureArray& b) {
        return a.Relev > b.Relev;
    }
};

struct TFeatureArrayVector : public TVector<TFeatureArray> {
    const size_t ArraySize;
    TVector<TVector<float> > Features;

    explicit TFeatureArrayVector(ui32 sz, ui32 segments)
        : ArraySize(sz)
    {
        Features.reserve(segments);
    }

    virtual ~TFeatureArrayVector() {}

    virtual void Calculate(const TDocContext&, const THeaderSpans&, const TSegmentSpans&) = 0;

    TFeatureArray& PushBack(ui32 num) {
        Features.push_back(TVector<float>());
        push_back(TFeatureArray(num, ArraySize, Features[size()]));
        return back();
    }

    void Print(IOutputStream& out) const {
        for (const_iterator it = begin(); it != end(); ++it) {
            out << CutWWWPrefix(GetOnlyHost(it->Url)) << '\t' <<    // queryId (host)
                   it->Relev << '\t' <<                             // relev (goal)
                   it->Number << " " <<                             // segment number
                   it->Lcs << " " <<                                // lcs between real content and content
                   it->RealContentSize << " " <<                    // real content size (words)
                   it->ContentSize << " " <<                        // segment size (words)
                   it->Url << '\t' <<                               // url
                   it->Weight;

            for (ui32 i = 0; i < ArraySize; ++i)
                out << '\t' << it->F[i];

            out << "\n";
        }
    }

    template <typename TMatrixNet, typename TWSpans>
    void SetWeights(TWSpans& sp) const {
        TVector<double> results;
        TMatrixNet::Apply(Features, results);
        size_t count = 0;
        for (const_iterator it = begin(); it != end(); ++it) {
            typename TWSpans::value_type& s = sp[it->Number];
            s.Weight = results[count++];
            s.MainWeight = f16::New(s.Weight).val;
        }
    }
};

struct TWeightComparator {
    template <typename TWSpan>
    bool operator()(const TWSpan& a, const TWSpan& b) const {
        return a.Weight > b.Weight;
    }
};

}
}
