#pragma once

#include <library/cpp/offroad/custom/ui64_varint_serializer.h>

namespace NOffroad {
    template <class Data>
    struct TOffsetData {
        TOffsetData() = default;
        TOffsetData(ui64 offset, const Data& base)
            : Offset(offset)
            , Base(base)
        {
        }

        ui64 Offset = 0;
        Data Base = Data();
    };

    template <class Serializer, class IntSerializer = TUi64VarintSerializer>
    class TOffsetDataSerializer: protected  Serializer {
    public:
        enum {
            MaxSize = Serializer::MaxSize + IntSerializer::MaxSize
        };

        template <class Data>
        static size_t Serialize(const TOffsetData<Data>& data, ui8* dst) {
            size_t size = 0;
            size += IntSerializer::Serialize(data.Offset, dst + size);
            size += Serializer::Serialize(data.Base, dst + size);
            return size;
        }

        template <class Data>
        static size_t Deserialize(const ui8* src, TOffsetData<Data>* data) {
            size_t size = 0;
            size += IntSerializer::Deserialize(src + size, &data->Offset);
            size += Serializer::Deserialize(src + size, &data->Base);
            return size;
        }
    };

}
