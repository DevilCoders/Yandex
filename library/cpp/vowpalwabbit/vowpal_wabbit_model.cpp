#include "vowpal_wabbit_model.h"

#include <util/generic/bitops.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NVowpalWabbit {
    static constexpr ui32 MIN_BITS = 10;
    static constexpr ui32 MAX_BITS = 32;

    TModel::TModel(const TBlob& weights)
        : Data(weights)
        , Weights(reinterpret_cast<const float*>(Data.AsCharPtr()))
        , Bits(static_cast<ui8>(MostSignificantBit(Data.Length() / sizeof(float))))
        , HashMask((1U << Bits) - 1)
    {
        CheckBlob();
    }

    TModel::TModel(const TString& flatWeightFileName)
        : TModel(TBlob::PrechargedFromFile(flatWeightFileName))
    {
    }

    void TModel::CheckBlob() {
        if (Bits < MIN_BITS || Bits > MAX_BITS || Data.Length() != (1L << Bits) * sizeof(float)) {
            ythrow yexception() << "Invalid model file size: " << Data.Length();
        }
    }

    const float* TModel::GetWeights() const {
        return Weights;
    }

    ui8 TModel::GetBits() const {
        return Bits;
    }

    size_t TModel::GetWeightsSize() const {
        return 1UL << Bits;
    }

    void TModel::ConvertReadableModel(const TString& readableModelFileName, const TString& flatWeightFileName) {
        auto model = ReadTextModel(readableModelFileName);
        const auto& blob = model.GetBlob();
        TFileOutput(flatWeightFileName).Write(blob.Data(), blob.Length());
    }

    const TBlob& TModel::GetBlob() const {
        return Data;
    }

    TModel ReadTextModel(const TString& fileName) {
        TFileInput reader(fileName);
        return ReadTextModel(reader);
    }

    TModel ReadTextModel(IInputStream& reader) {
        TString line;
        reader.ReadLine(line);

        Y_ENSURE(line.StartsWith("Version 8."), "Unsupported version: \"" << line << '"');

        reader.ReadLine(line);
        if (line.StartsWith("Id")) { // new in 8.2.0
            reader.ReadLine(line);
        }
        Y_ENSURE(line.StartsWith("Min label:"), "Unexpected line: \"" << line << '"');

        reader.ReadLine(line);
        Y_ENSURE(line.StartsWith("Max label:"), "Unexpected line: \"" << line << '"');

        reader.ReadLine(line);
        Y_ENSURE(line.StartsWith("bits:"), "Unexpected line: \"" << line << '"');

        const ui32 bits = FromString<ui32>(line.substr(5));
        Y_ENSURE(bits >= MIN_BITS || bits <= MAX_BITS, "Invalid bits value: " << bits);

        size_t size = 0;
        for (size = reader.ReadLine(line); size; size = reader.ReadLine(line)) {
            const size_t pos = line.find(':', 0);
            Y_ENSURE(TString::npos != pos, "Invalid line: \"" << line << '"');

            if (line.find_first_not_of(TStringBuf("0123456789")) == pos && pos > 0) {
                break;
            }
        }

        Y_ENSURE(size > 0, "Unexpected end of file");

        TVector<float> arr(1L << bits, 0.0f);
        while (size != 0) {
            auto index = ui32{};
            auto val = float{};
            Split(line, ':', index, val);

            Y_ENSURE(index < arr.size(), "Invalid index: " << index);
            Y_ENSURE(!arr[index], "Value at index " << index << " already set");

            arr[index] = val;
            size = reader.ReadLine(line);
        }
        return TModel(TBlob::Copy(arr.data(), arr.size() * sizeof(arr.front())));
    }

    TPackedModel::TPackedModel(const TBlob& model)
        : Data(model)
        , Weights(reinterpret_cast<const i8*>(Data.AsCharPtr() + sizeof(float)))
        , Size(Data.Size() - sizeof(float))
        , Bits(static_cast<ui8>(MostSignificantBit(Size)))
        , HashMask((1U << Bits) - 1)
    {
        Y_ENSURE(Data.Size() > sizeof(float));
        Multiplier = reinterpret_cast<const float*>(Data.AsCharPtr())[0];
        size_t size = Size;
        while (size % 2 == 0) {
            size /= 2;
        }

        Y_ENSURE(size == 1, "Invalid Vowpal Wabbit hash table size: " << Size);
    }

    TPackedModel::TPackedModel(const TString& modelFileName)
        : TPackedModel(TBlob::PrechargedFromFile(modelFileName))
    {
    }
}
