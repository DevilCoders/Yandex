#include "builder.h"

#include <library/cpp/compproto/huff.h>
#include <library/cpp/compproto/metainfo.h>
#include <library/cpp/compproto/bit.h>

#include <library/cpp/containers/comptrie/comptrie_builder.h>

#include <util/stream/file.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/string/split.h>

template<class TMeta, class TBuffer>
bool Serialize(const NWordFeatures::TSurfFeatures& features, TMeta& meta, TBuffer& buffer)
{
    meta.BeginSelf(buffer);

    bool somethingDumped = false;

    for (size_t i = 0; i < NWordFeatures::SURF_TOTAL; ++i) {
        const ui32 f = features.Features[i] * 10000.0;
        if (f == 0) {
            continue;
        }

        meta.BeginElement(i, buffer);
        meta.SetScalar(0, f, buffer);
        meta.EndElement(buffer);

        somethingDumped = true;
    }

    meta.EndRepeated(buffer);

    return somethingDumped;
}

class TOptimizedMetaBuilder : public TNonCopyable {
public:
    typedef NCompProto::TMetaInfo<NCompProto::THist> TMetaInfo;
private:
    THolder<TMetaInfo> Meta;
    bool Finished;
    TStringStream OptimizedMeta;
public:
    TOptimizedMetaBuilder(const TString& meta = NWordFeatures::SURF_META_INFO)
        : Finished(false)
    {
        Meta.Reset(new TMetaInfo(meta));
    }

    void Dump(const TString& /*word*/, const NWordFeatures::TSurfFeatures& features)
    {
        NCompProto::TEmpty buffer;
        Serialize(features, *Meta, buffer);
    }

    void Finish()
    {
        typedef NCompProto::TMetaInfo<NCompProto::THuff> TMetaLocal;

        THolder<TMetaLocal> meta(new TMetaLocal(*Meta, NCompProto::THistToHuff::Instance()));
        meta->Save(OptimizedMeta);
        meta.Reset(nullptr);

        Finished = true;
    }

    TString GetOptimizedMeta() const
    {
        if (!Finished) {
            ythrow yexception() << "Not finished yet!";
        }
        return OptimizedMeta.Str();
    }
};

class TSurfFeaturesFileIterator {
private:
    TAutoPtr<IInputStream> In;

    TString Line;
    TVector<TStringBuf> Tmp;

    TString Key;
    NWordFeatures::TSurfFeatures Features;

    bool Valid;
public:
    TSurfFeaturesFileIterator(const TString& inputFileName)
        : In(OpenMaybeCompressedInput(inputFileName))
        , Valid(true)
    {
        ++(*this);
    }

    bool IsValid() const
    {
        return Valid;
    }

    const TString& GetKey() const
    {
        return Key;
    }

    const NWordFeatures::TSurfFeatures& GetFeatures() const
    {
        return Features;
    }

    void operator++()
    {
        if (!Valid) {
            return;
        }

        if (In->ReadLine(Line)) {
            Tmp.clear();
            StringSplitter(Line.data(), Line.data() + Line.size()).Split('\t').AddTo(&Tmp);
            Y_VERIFY(Tmp.size() == NWordFeatures::SURF_TOTAL + 1, "Not enough fields!");

            Key = TString{Tmp[0]};

            for (size_t i = 0; i < NWordFeatures::SURF_TOTAL; ++i) {
                Features.Features[i] = FromString<float>(Tmp[i + 1]);
            }
        } else {
            Valid = false;
        }
    }
};

class TSurfCompprotoPacker {
    typedef NCompProto::TMetaInfo<NCompProto::THuff> TMetaHuffInfo;
    typedef NCompProto::TMetaInfo<NCompProto::TTable> TMetaTableInfo;

    TAtomicSharedPtr<TMetaTableInfo> MetaUnpackInfo;
public:
    typedef TArrayRef<const char> TData;

    TSurfCompprotoPacker()
    {
    }

    TSurfCompprotoPacker(const TString& metaHuffInfoStr)
        : MetaUnpackInfo(
            new TMetaTableInfo(
                TMetaHuffInfo(metaHuffInfoStr),
                NCompProto::THuffToTable::Instance()
            )
          )
    {
    }

    size_t SkipLeaf(const char* buffer) const
    {
        Y_ASSERT(MetaUnpackInfo.Get() != nullptr);

        const ui8* p = (const ui8*)buffer;
        ui64 offset = 0;
        NCompProto::GetEmptyDecompressor().Decompress(MetaUnpackInfo.Get(), p, offset);
        // Indexregex with strictly 1 record may have 0 bytes leaf size.
        return Max<size_t>((offset + 7) / 8, 1);
    }

    void PackLeaf(char* buffer, const TData& data, size_t computedSize) const
    {
        memcpy(buffer, data.data(), computedSize);
    }

    size_t MeasureLeaf(const TData& data) const
    {
        return data.size();
    }

    void UnpackLeaf(const char* /*buffer*/, TData& /*result*/) const
    {
        // Compiler needs method to exist
        ythrow yexception() << "Not expected to be called!";
    }
};

typedef TCompactTrieBuilder<char, TSurfCompprotoPacker::TData, TSurfCompprotoPacker> TSurfTrieWriter;

namespace NWordFeatures {

    void BuildSurfIndex(const TString& srcFile, const TString& outputFile, bool optimizeTrie)
    {
        TString optimizedMeta = "";

        TSurfFeatures meanFeatures;
        ui32 count = 0;

        // Phase 1. Building optimized meta
        {
            TOptimizedMetaBuilder builder(SURF_META_INFO);
            TSurfFeaturesFileIterator iter(srcFile);

            for (ui32 index = 0; iter.IsValid(); ++iter, ++index) {
                if (index % 1000 == 0) {
                    meanFeatures += iter.GetFeatures();
                    ++count;
                }
                if (iter.GetKey().size() >= MaxKeyLen) {
                    continue;
                }
                builder.Dump(iter.GetKey(), iter.GetFeatures());
            }

            builder.Finish();

            optimizedMeta = builder.GetOptimizedMeta();
        }

        meanFeatures *= (1.0 / count);

        // Phase 2. Building index
        {
            TSurfCompprotoPacker packer(optimizedMeta);
            TSurfTrieWriter writer(CTBF_PREFIX_GROUPED, packer);
            NCompProto::TMetaInfo<NCompProto::THuff> MetaHuff(optimizedMeta);

            TFixedBufferFileOutput out(outputFile);

            TBufferOutput bufferForML;

            // Saving meta.
            const ui32 metaLength = optimizedMeta.size();
            Save(&out, metaLength);
            out.Write(optimizedMeta.data(), optimizedMeta.size());
            {
                const ui32 surfTotal = SURF_TOTAL;
                Save(&out, surfTotal);
            }
            SaveIterRange(&out, meanFeatures.Features, meanFeatures.Features + SURF_TOTAL);

            TSurfFeaturesFileIterator iter(srcFile);

            for (; iter.IsValid(); ++iter) {
                if (iter.GetKey().size() >= MaxKeyLen) {
                    continue;
                }

                NCompProto::TBitBuffer buffer;
                const bool ok = Serialize(iter.GetFeatures(), MetaHuff, buffer);
                const TArrayRef<const char> data = buffer.AsDataRegion();
                if (ok) {
                    if (Y_LIKELY(data.size() > 0)) {
                        writer.Add(iter.GetKey().data(), iter.GetKey().size(), data);
                    } else {
                        const char fake = 0;
                        const TArrayRef<const char> oneByteFakeData(&fake, 1);
                        writer.Add(iter.GetKey().data(), iter.GetKey().size(), data);
                    }
                }
            }

            if (optimizeTrie) {
                TBufferOutput buffer;
                writer.SaveAndDestroy(buffer);
                CompactTrieMinimize<TSurfCompprotoPacker>(
                    bufferForML,
                    buffer.Buffer().Data(),
                    buffer.Buffer().Size(),
                    false,
                    packer,
                    NCompactTrie::MM_DEFAULT
                );
            } else {
                writer.SaveAndDestroy(bufferForML);
            }

            // Minimize layout.
            {
                CompactTrieMakeFastLayout<TSurfCompprotoPacker>(
                    out,
                    bufferForML.Buffer().Data(),
                    bufferForML.Buffer().Size(),
                    false,
                    packer
                );
            }
        }
    }

}; // NWordFeatures
