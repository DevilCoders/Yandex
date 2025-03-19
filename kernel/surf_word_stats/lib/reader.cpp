#include "reader.h"

#include <library/cpp/compproto/huff.h>
#include <library/cpp/compproto/metainfo.h>
#include <library/cpp/compproto/bit.h>

#include <library/cpp/containers/comptrie/comptrie_trie.h>

#include <util/stream/file.h>

#include <util/memory/blob.h>

#include <util/ysaveload.h>

static const TVector<TString> GenerateSurfFeatureNames()
{
    TVector<TString> raw;

    for (int i = 0; i < static_cast<int>(NWordFeatures::SURF_TOTAL); ++i) {
        raw.push_back(ToString(static_cast<NWordFeatures::ESurfFeatureType>(i)));
    }

    TVector<TString> result;
    for (const auto& x : raw) {
        result.push_back(x + "_MEAN");
        result.push_back(x + "_MIN");
        result.push_back(x + "_MAX");
    }

    return result;
}

static const TVector<TString> SurfFeatureNames = GenerateSurfFeatureNames();

struct TSurfFeaturesDecompressor : TNonCopyable, NCompProto::TParentHold<TSurfFeaturesDecompressor> {
private:
    NWordFeatures::TSurfFeatures* Features;

    ui32 CurrentIndex;

    NCompProto::TEmptyDecompressor Empty;
public:
    TSurfFeaturesDecompressor()
    {
    }

    inline void Init(NWordFeatures::TSurfFeatures* features)
    {
        Features = features;
    }

    inline void BeginSelf(ui32& /*index*/, ui32 /*id*/)
    {
    }

    inline void EndSelf()
    {
    }

    inline void BeginElement(ui32 val)
    {
        CurrentIndex = val;
    }

    inline void EndElement()
    {
    }

    inline void SetScalar(size_t index, ui32 val)
    {
        Y_ASSERT(index == 0);
        if (Y_LIKELY(CurrentIndex < NWordFeatures::SURF_TOTAL)) {
            Features->Features[CurrentIndex] = val / 10000.0f;
        }
    }

    NCompProto::IDecompressor& GetDecompressor(size_t index)
    {
        return Empty.GetDecompressor(index);
    }
};

typedef NCompProto::TMetaInfo<NCompProto::THuff> TMetaInfo;
typedef NCompProto::TMetaInfo<NCompProto::TTable> TIndexDecompressor;

class TSurfCompprotoPacker {
private:
    TIndexDecompressor* Decompressor;
public:
    typedef NWordFeatures::TSurfFeatures TData;

    TSurfCompprotoPacker() = default;

    TSurfCompprotoPacker(TIndexDecompressor* decompressor)
        : Decompressor(decompressor)
    {
    }

    TSurfCompprotoPacker(const TSurfCompprotoPacker&) = default;

    TSurfCompprotoPacker& operator=(const TSurfCompprotoPacker&) = default;

    size_t SkipLeaf(const char* buffer) const
    {
        const ui8* p = (const ui8*)buffer;
        ui64 offset = 0;
        NCompProto::GetEmptyDecompressor().Decompress(Decompressor, p, offset);
        // Indexregex with strictly 1 record may have 0 bytes leaf size.
        return Max<size_t>((offset + 7) / 8, 1);
    }

    void UnpackLeaf(const char* buffer, TData& result) const
    {
        const ui8* p = (const ui8*)buffer;
        ui64 offset = 0;
        NCompProto::TMetaIterator<TSurfFeaturesDecompressor> metaDecompressor;
        metaDecompressor.Self.Init(&result);
        metaDecompressor.Decompress(Decompressor, p, offset);
    }
};

namespace NWordFeatures {

    class TSurfFeaturesCalcer::TImpl {
    private:
        TString Meta;

        THolder<TMetaInfo>  MetaInfo;
        THolder<TIndexDecompressor> Decompressor;

        typedef TCompactTrie<char,NWordFeatures::TSurfFeatures,TSurfCompprotoPacker> TSurfFeatureTrie;
        TBlob Blob;
        THolder<TSurfCompprotoPacker> Packer;
        THolder<TSurfFeatureTrie> Trie;

        TSurfFeatures MeanFeatures;
    public:
        TImpl(IInputStream& in)
        {
            // Reading meta.
            {
                ui32 metaSize = 0;
                Load(&in, metaSize);

                TVector<char> metaV(metaSize);
                in.LoadOrFail(&metaV[0], metaSize);
                Meta.assign(metaV.begin(), metaV.end());
            }

            // Reading mean one.
            {
                ui32 numFeatures = 0;
                Load(&in, numFeatures);

                if (Y_LIKELY(numFeatures == SURF_TOTAL)) {
                    LoadIterRange(&in, MeanFeatures.Features, MeanFeatures.Features + SURF_TOTAL);
                } else {
                    float fake = 0.0f;
                    for (size_t i = 0; i < numFeatures; ++i) {
                        if (i < SURF_TOTAL) {
                            Load(&in, MeanFeatures.Features[i]);
                        } else {
                            Load(&in, fake);
                        }
                    }
                }
            }

            TStringInput stream(Meta);
            MetaInfo.Reset(new TMetaInfo(stream));
            Decompressor.Reset(new TIndexDecompressor(*MetaInfo, NCompProto::THuffToTable::Instance()));

            Blob = TBlob::FromStream(in);
            Packer.Reset(new TSurfCompprotoPacker(Decompressor.Get()));
            Trie.Reset(new TSurfFeatureTrie(Blob, *Packer));
        }

        bool Find(const TStringBuf& word, TSurfFeatures& features) const
        {
            features.Clear();
            return Trie->Find(word.data(), word.size(), &features);
        }

        const TSurfFeatures& GetMean() const
        {
            return MeanFeatures;
        }
    };

    TSurfFeaturesCalcer::TSurfFeaturesCalcer(const TString& indexFile)
    {
        TFileInput in(indexFile);
        Impl = MakeHolder<TImpl>(in);
    }

    TSurfFeaturesCalcer::TSurfFeaturesCalcer(IInputStream& in)
        : Impl(new TImpl(in))
    {
    }

    bool TSurfFeaturesCalcer::Find(const TStringBuf& word, TSurfFeatures& features) const
    {
        return Impl->Find(word, features);
    }

    const TSurfFeatures& TSurfFeaturesCalcer::GetMean() const
    {
        return Impl->GetMean();
    }

    bool TSurfFeaturesCalcer::FindOrMean(const TStringBuf& word, TSurfFeatures& features) const
    {
        const bool ok = Impl->Find(word, features);
        if (!ok) {
            features = GetMean();
        }
        return ok;
    }

    const TVector<TString>& TSurfFeaturesCalcer::GetQueryFeatureNames()
    {
        return SurfFeatureNames;
    }

    ui32 TSurfFeaturesCalcer::GetQueryFeaturesCount()
    {
        return SurfFeatureNames.size();
    }

    ui32 TSurfFeaturesCalcer::GetFeatureOffset(ESurfFeatureType featureType, ESurfStatisticType statisticType)
    {
        return SURF_STAT_TOTAL * (ui32)featureType + (ui32)statisticType;
    }

    void TSurfFeaturesCalcer::FillQueryFeatures(const TStringBuf& request, TVector<float>& features) const
    {
        features.resize((size_t)SURF_TOTAL * (size_t)SURF_STAT_TOTAL);
        FillQueryFeatures(request, &features[0], features.size());
    }

    void TSurfFeaturesCalcer::FillQueryFeatures(const TStringBuf& request, float* features, size_t maxFeatures) const
    {
        TSurfFeatures mean;
        TSurfFeatures min;
        TSurfFeatures max;

        for (size_t i = 0; i < SURF_TOTAL; ++i) {
            min.Features[i] = 1.0f;
        }

        ui32 countWords = 0;

        TSurfFeatures current;

        TStringBuf rc = request;
        for (; !rc.empty(); ++countWords) {
            const TStringBuf word = rc.NextTok(' ');
            const bool ok = FindOrMean(word, current);
            mean += current;
            if (ok) {
                for (size_t i = 0; i < SURF_TOTAL; ++i) {
                    min.Features[i] = Min<float>(min.Features[i], current.Features[i]);
                    max.Features[i] = Max<float>(max.Features[i], current.Features[i]);
                }
            }
        }

        if (countWords) {
            mean *= 1.0 / countWords;
        }

        const size_t realMaxFeatures = Min<size_t>(GetQueryFeaturesCount(), maxFeatures);
        for (size_t i = 0, k = 0; i + SURF_STAT_TOTAL <= realMaxFeatures; i += SURF_STAT_TOTAL, ++k) {
            features[i + SURF_STAT_MEAN] = mean.Features[k];
            features[i + SURF_STAT_MIN]  = min.Features[k];
            features[i + SURF_STAT_MAX]  = max.Features[k];
        }
    }

    TSurfFeaturesCalcer::~TSurfFeaturesCalcer()
    {
    }
};
