#pragma once
#include <library/cpp/logger/global/global.h>
#include <util/generic/vector.h>
#include <util/system/unaligned_mem.h>
#include <util/system/yassert.h>
#include <utility>

class TBitsReader {
public:
    enum TFieldType {
        ftInt = 0,
        ftFloat = 1
    };

    struct TFieldDescr {
        ui8 Width;
        TFieldType Type;
        ui32 IndexRemap;
    };
private:
    enum TTrivialCase {
        tcNotTrivial = 0,
        tcWhole = 1,
        tcBits = 2
    };
    TVector<TFieldDescr> Fields;
    TFieldDescr* FieldsData;
    ui32 SumWidth;
    TTrivialCase TrivialCase;
    bool IsChunk;
    ui32 IndexRemap;
    TFieldType Type;
    ui32 FieldsSize;

    template <class TStorage>
    inline void UnPackTrivial(float* data, const TStorage* storagePtr) const {
        Y_ASSERT(sizeof(TStorage) * 8 == SumWidth);
        TStorage storage = ReadUnaligned<TStorage>(storagePtr);
        if (Type == ftFloat) {
            data[IndexRemap] = (float)storage / (float)((1 << SumWidth) - 1);
        } else {
            data[IndexRemap] = storage;
        }
    }

    template <class TStorage>
    inline void PackTrivial(const float* data, TStorage* storagePtr) const {
        Y_ASSERT(sizeof(TStorage) * 8 == SumWidth);
        float valueF = data[IndexRemap];
        TStorage storage;
        if (Type == ftFloat) {
            VERIFY_WITH_LOG(valueF <= 1 && valueF >= 0, "Incorrect valueF");
            storage = valueF * ((1 << SumWidth) - 1);
        } else {
            storage = data[IndexRemap];
        }
        WriteUnaligned<TStorage>(storagePtr, storage);
    }

    inline void UnPackTrivial(float* data, const ui32* storagePtr) const {
        memcpy(&data[IndexRemap], storagePtr, SumWidth / 8);
    }

    inline void PackTrivial(const float* data, ui32* storagePtr) const {
        memcpy(storagePtr, &data[IndexRemap], SumWidth / 8);
    }

    inline void UnPackTrivial(float* data, const ui64* storagePtr) const {
        memcpy(&data[IndexRemap], storagePtr, SumWidth / 8);
    }

    inline void PackTrivial(const float* data, ui64* storagePtr) const {
        memcpy(storagePtr, &data[IndexRemap], SumWidth / 8);
    }

    template <class TStorage>
    inline void UnPackBits(float* /*data*/, const TStorage* /*storagePtr*/) const {
        FAIL_LOG("Incorrect class usage");
    }

    inline void UnPackBits(float* data, const ui8* storagePtr) const {
        auto storage = ReadUnaligned<ui8>(storagePtr);
        data[FieldsData[0].IndexRemap] = ((storage & (1 << 7)) ? 1 : 0);
        data[FieldsData[1].IndexRemap] = ((storage & (1 << 6)) ? 1 : 0);
        data[FieldsData[2].IndexRemap] = ((storage & (1 << 5)) ? 1 : 0);
        data[FieldsData[3].IndexRemap] = ((storage & (1 << 4)) ? 1 : 0);
        data[FieldsData[4].IndexRemap] = ((storage & (1 << 3)) ? 1 : 0);
        data[FieldsData[5].IndexRemap] = ((storage & (1 << 2)) ? 1 : 0);
        data[FieldsData[6].IndexRemap] = ((storage & (1 << 1)) ? 1 : 0);
        data[FieldsData[7].IndexRemap] = ((storage & (1 << 0)) ? 1 : 0);
    }

    template <class TStorage>
    inline void PackBits(const float* /*data*/, TStorage* /*storagePtr*/) const {
        FAIL_LOG("Incorrect class usage");
    }

    inline void PackBits(const float* data, ui8* storagePtr) const {
        ui8 storage = 0;
        storage |= (data[FieldsData[0].IndexRemap] ? 1 << 7 : 0);
        storage |= (data[FieldsData[1].IndexRemap] ? 1 << 6 : 0);
        storage |= (data[FieldsData[2].IndexRemap] ? 1 << 5 : 0);
        storage |= (data[FieldsData[3].IndexRemap] ? 1 << 4 : 0);
        storage |= (data[FieldsData[4].IndexRemap] ? 1 << 3 : 0);
        storage |= (data[FieldsData[5].IndexRemap] ? 1 << 2 : 0);
        storage |= (data[FieldsData[6].IndexRemap] ? 1 << 1 : 0);
        storage |= (data[FieldsData[7].IndexRemap] ? 1 << 0 : 0);
        WriteUnaligned<ui8>(storagePtr, storage);
    }

    template <class TStorage>
    inline void UnPackNonTrivial(float* data, const TStorage* storagePtr) const {
        Y_ASSERT(sizeof(TStorage) * 8 >= SumWidth);
        TStorage storage = ReadUnaligned<TStorage>(storagePtr);
        for (ui32 i = 0; i < FieldsSize; ++i) {
            const TFieldDescr& r = FieldsData[i];
            TStorage val = storage >> (sizeof(TStorage) * 8 - r.Width);
            if (r.Type == ftFloat) {
                data[r.IndexRemap] = (float)(val) / ((float)((1 << r.Width) - 1));
            } else {
                data[r.IndexRemap] = val;
            }
            storage <<= r.Width;
        }
    }

    template <class TStorage>
    inline void PackNonTrivial(const float* data, TStorage* storagePtr) const {
        Y_ASSERT(sizeof(TStorage) * 8 >= SumWidth);
        TStorage storage = 0;
        ui32 bs = 0;
        TStorage val = 0;
        for (ui32 i = 0; i < FieldsSize; ++i) {
            const TFieldDescr& r = FieldsData[i];
            float valueF = data[r.IndexRemap];
            if (r.Type == ftFloat) {
                VERIFY_WITH_LOG(valueF <= 1 && valueF >= 0, "Incorrect valueF");
                val = ((1 << r.Width) - 1) * valueF;
            } else {
                val = valueF;
            }
            storage |= (val << (sizeof(TStorage) * 8 - r.Width - bs));
            bs += r.Width;
        }
        WriteUnaligned<TStorage>(storagePtr, storage);
    }

public:

    TBitsReader() {

    }

    TBitsReader(const TBitsReader& orig) {
        *this = orig;
        FieldsData = &Fields[0];
    }

    TBitsReader& operator=(const TBitsReader& orig) = default;

    TBitsReader(const TVector<TFieldDescr>& fields)
        : IsChunk(false)
    {
        Fields = fields;
        FieldsData = &Fields[0];
        SumWidth = 0;
        FieldsSize = fields.size();
        TrivialCase = tcNotTrivial;
        for (ui32 i = 0; i < fields.size(); ++i) {
            SumWidth += fields[i].Width;
            VERIFY_WITH_LOG(fields[i].Width <= 32, "Incorrect bit field width");
            VERIFY_WITH_LOG(fields[i].Width, "Incorrect width");
        }

        if (((SumWidth == 8) || (SumWidth == 16) || (SumWidth == 32) || (SumWidth == 64)) && (Fields.size() == 1)) {
            TrivialCase = tcWhole;
            IndexRemap = Fields[0].IndexRemap;
            Type = Fields[0].Type;
            IsChunk = (SumWidth == 32);
        } else if (SumWidth == 8 && Fields.size() == 8) {
            TrivialCase = tcBits;
        }
    }

    bool MergeChunks(TBitsReader& reader) {
        if (TrivialCase != tcWhole || reader.TrivialCase != tcWhole)
            return false;
        if (IsChunk && reader.IsChunk && (reader.IndexRemap - (SumWidth / 32 - 1) - IndexRemap) == 1) {
            SumWidth += reader.SumWidth;
            for (ui32 copy = 0; copy < reader.Fields.size(); ++copy) {
                Fields.push_back(reader.Fields[copy]);
            }
            FieldsData = &Fields[0];
            FieldsSize = Fields.size();
            return true;
        }
        return false;
    }

    template <class TStorage>
    inline void UnPack(float* data, const TStorage* storagePtr) const {
        switch (TrivialCase) {
        case tcNotTrivial:
            UnPackNonTrivial(data, storagePtr);
            break;
        case tcWhole:
            UnPackTrivial(data, storagePtr);
            break;
        case tcBits:
            UnPackBits(data, storagePtr);
            break;
        default:
            FAIL_LOG("Incorrect trivial detector");
        }
    }

    template <class TStorage>
    inline void Pack(const float* data, TStorage* storagePtr) const {
        switch (TrivialCase) {
        case tcNotTrivial:
            PackNonTrivial(data, storagePtr);
            break;
        case tcWhole:
            PackTrivial(data, storagePtr);
            break;
        case tcBits:
            PackBits(data, storagePtr);
            break;
        default:
            FAIL_LOG("Incorrect trivial detector");
        }
    }

};

class TBitsRemapper {
private:
    TVector<TBitsReader::TFieldDescr> Fields;
    TVector<TBitsReader> Readers;
    TVector<ui8> Sizes;
    ui8* SizesData;
    TBitsReader* ReadersData;
    ui32 SizesSize;

public:

    TBitsRemapper(const TVector<TBitsReader::TFieldDescr>& fields) {
        Fields = fields;
        VERIFY_WITH_LOG(fields.size(), "Incorrect width array");
        ui32 size = 0;
        TVector<TBitsReader::TFieldDescr> fieldsCurrent;
        for (ui32 i = 0; i < Fields.size(); ++i) {
            size += Fields[i].Width;
            fieldsCurrent.push_back(Fields[i]);
            if (size == 8 || size == 16 || size == 32 || size == 64 || i == Fields.size() - 1) {
                VERIFY_WITH_LOG(size && size <= 64, "Incorrect size value");
                Readers.push_back(TBitsReader(fieldsCurrent));
                Sizes.push_back(size / 8 + (size % 8 ? 1 : 0));
                size = 0;
                fieldsCurrent.clear();
            }
        }

        for (ui32 reader = 0; reader < Readers.size() - 1;) {
            if (Readers[reader].MergeChunks(Readers[reader + 1])) {
                Sizes[reader] += Sizes[reader + 1];
                Readers.erase(Readers.begin() + reader + 1);
                Sizes.erase(Sizes.begin() + reader + 1);
            } else
                ++reader;
        }
        ReadersData = &Readers[0];
        SizesData = &Sizes[0];
        SizesSize = Sizes.size();
    }

    TBitsRemapper(const TBitsRemapper& orig) {
        *this = orig;
        ReadersData = &Readers[0];
        SizesData = &Sizes[0];
    }

    TBitsRemapper& operator=(const TBitsRemapper& orig) = default;

    inline void UnPack(float* data, const ui8* storage) const {
        for (ui32 i = 0; i < SizesSize; ++i) {
            switch (SizesData[i]) {
            case 1:
                ReadersData[i].UnPack<ui8>(data, storage);
                storage += 1;
                break;
            case 2:
                ReadersData[i].UnPack<ui16>(data, (const ui16*)storage);
                storage += 2;
                break;
            case 4:
                ReadersData[i].UnPack<ui32>(data, (const ui32*)storage);
                storage += 4;
                break;
            case 8:
                ReadersData[i].UnPack<ui64>(data, (const ui64*)storage);
                storage += 8;
                break;
            default:
                Y_ASSERT(SizesData[i] % 4 == 0);
                Readers[i].UnPack<ui32>(data, (const ui32*)storage);
                storage += SizesData[i];
            }
        }
    }

    inline void Pack(const float* data, ui8* storage) const {
        for (ui32 i = 0; i < Sizes.size(); ++i) {
            switch (Sizes[i]) {
            case 1:
                Readers[i].Pack<ui8>(data, storage);
                storage += 1;
                break;
            case 2:
                Readers[i].Pack<ui16>(data, (ui16*)storage);
                storage += 2;
                break;
            case 4:
                Readers[i].Pack<ui32>(data, (ui32*)storage);
                storage += 4;
                break;
            case 8:
                Readers[i].Pack<ui64>(data, (ui64*)storage);
                storage += 8;
                break;
            default:
                Y_ASSERT(Sizes[i] % 4 == 0);
                Readers[i].Pack<ui32>(data, (ui32*)storage);
                storage += Sizes[i];
            }
        }
    }
};
