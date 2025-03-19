#include "factor_storage.h"

#include "factors_reader.h"

#include <library/cpp/codecs/float_huffman.h>

#include <util/stream/str.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>

using namespace NFactorSlices;

const TFactorDomain TFactorStorage::EmptyDomain;

void TGeneralResizeableFactorStorage::Save(IOutputStream* rh) const
{
    ::Save(rh, FactorsCount);
    ::SaveArray(rh, factors, FactorsCount);
}

void TGeneralResizeableFactorStorage::Save(IOutputStream* rh, size_t i) const
{
    ::Save(rh, factors[i]);
}

void TGeneralResizeableFactorStorage::Load(IInputStream* rh)
{
    ui32 cnt;
    ::Load(rh, cnt);
    Resize(cnt);
    ::LoadArray(rh, factors, cnt);
}

void TGeneralResizeableFactorStorage::FloatMove(float* fromBegin, float* fromEnd, ptrdiff_t offset)
{
    if (0 == offset) {
        return;
    }

    Y_ASSERT(fromBegin <= fromEnd);

    float* toBegin = fromBegin + offset;
    float* toEnd = fromEnd + offset;

    if (offset > 0) {
        Rotate(fromBegin, fromEnd, toEnd);
    } else {
        Rotate(toBegin, fromBegin, fromEnd);
    }
}

TVector<EFactorSlice> GetFactorSlices(const TFactorStorage& storage) {
    return GetFactorSlices(storage.GetBorders());
}

TString SerializeFactorBorders(const TFactorStorage& storage)
{
    return SerializeFactorBorders(storage.GetBorders());
}

namespace NFSSaveLoad {
    namespace NPrivate {
        const ui8 CurVersion = 1;
    } // NPrivate

    void Serialize(const TFactorStorage& storage, IOutputStream* output)
    {
        Y_ASSERT(output);
        if (Y_UNLIKELY(!output)) {
            return;
        }

        output->Write(&NPrivate::CurVersion, 1);
        const NFactorSlices::TFactorBorders& borders = storage.GetBorders();
        TString fb = SerializeFactorBorders(borders);
        ::Save(output, fb);
        ::Save(output, NCodecs::NFloatHuff::Encode({storage.Ptr(0), storage.Size()}));
    }

    void Deserialize(const TStringBuf& bordersStr, const TStringBuf& huffStr,
        const NFactorSlices::TSlicesMetaInfo& hostInfo, TFactorStorage* dst)
    {
        Y_ASSERT(dst);
        if (Y_UNLIKELY(!dst)) {
            return;
        }

        auto reader = CreateHuffmanReader(bordersStr, huffStr);
        reader->ReadTo(*dst, hostInfo);
    }

    void Deserialize(IInputStream* input, const NFactorSlices::TSlicesMetaInfo& hostInfo, TFactorStorage* dst)
    {
        Y_ASSERT(dst);
        if (Y_UNLIKELY(!dst)) {
            return;
        }

        TVector<ui8> codedFactors;
        auto reader = CreateCompressedFactorsReader(input, codedFactors);
        reader->ReadTo(*dst, hostInfo);
    }
} // namespace NFSSaveLoad


NFSSaveLoad::TAppendFeaturesResult NFSSaveLoad::AppendFeaturesToSlice(
    TArrayRef<const float> inputFeatures,
    TStringBuf inputSliceString,
    EFactorSlice sliceToAppendInto,
    TArrayRef<const float> newFeatures,
    bool skipSlicesAndFeatsMissmatch)
{
    TAppendFeaturesResult result;

    size_t featuresToAppend = newFeatures.size();

    NFactorSlices::TFactorDomain inputDomain;
    NFactorSlices::TFactorDomain outputDomain;
    {
        NFactorSlices::TFactorBorders inputBorders;
        if (inputSliceString) {
            try {
                NFactorSlices::DeserializeFactorBorders(inputSliceString, inputBorders);
                Y_ENSURE(NFactorSlices::NDetail::ReConstruct(inputBorders));
                Y_ENSURE(inputBorders.SizeAll() == inputFeatures.size());
            } catch (...) {
                if (skipSlicesAndFeatsMissmatch) {
                    inputSliceString = "";
                } else {
                    throw;
                }
            }
        }

        if (!inputSliceString) {
            inputDomain = NFactorSlices::TFactorDomain(inputFeatures.size());
        } else {
            inputDomain = NFactorSlices::TFactorDomain(inputBorders);
        }

        outputDomain = inputDomain.MakeDomainWithIncreasedSlice(sliceToAppendInto, featuresToAppend);
    }

    TFactorStorage storage(outputDomain);
    {
        NFSSaveLoad::TFactorsReader reader(
            inputDomain.GetBorders(),
            NFSSaveLoad::CreateRawFloatsInput(inputFeatures.begin(), inputFeatures.size()));

        NFactorSlices::TSlicesMetaInfo meta;
        Y_ENSURE(NFactorSlices::NDetail::ReConstructMetaInfo(outputDomain.GetBorders(), meta));
        reader.ReadTo(storage, meta);
    }

    {
        auto view = storage.CreateView();
        ui32 shift = 0;
        if (inputDomain.GetBorders()[sliceToAppendInto].Empty()) {
            shift = outputDomain.GetBorders()[sliceToAppendInto].Begin;
        } else {
            shift = ui32(outputDomain.GetBorders()[sliceToAppendInto].Begin) +
            inputDomain.GetBorders()[sliceToAppendInto].End -
            inputDomain.GetBorders()[sliceToAppendInto].Begin;
        }
        for (auto i : xrange(featuresToAppend)) {
            view[shift + i] = newFeatures[i];
        }
    }

    {
        auto view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALL);
        result.Features.assign(view.begin(), view.end());
    }
    result.Borders = NFactorSlices::SerializeFactorBorders(
        outputDomain.GetBorders(), NFactorSlices::ESerializationMode::LeafOnly
    );
    if (inputDomain.GetBorders()[sliceToAppendInto].Empty()) {
        result.AppendedFeaturesBegin = outputDomain[sliceToAppendInto].Begin;
    } else {
        result.AppendedFeaturesBegin = inputDomain[sliceToAppendInto].End;
    }
    result.AppendedFeaturesEnd = outputDomain[sliceToAppendInto].End;
    return result;
}
