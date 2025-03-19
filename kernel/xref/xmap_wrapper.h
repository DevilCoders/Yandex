#pragma once

#include "xmap.h"

#include <library/cpp/on_disk/4d_array/array4d_poly.h>
#include <library/cpp/on_disk/4d_array/memory4d_poly.h>

class TXMapWrapper : public TXMapEnums {
private:
    TArray4DPoly::TItemsLayer Layer;
    const TXMapInfo* LegacyXmapInfo;

private:
    inline void Init(const TArray4DPoly::TElementsLayer& elementsLayer, size_t brId, size_t regId) {
        if (brId < elementsLayer.GetCount()) {
            const TArray4DPoly::TEntriesLayer entryLayer = elementsLayer.GetSubLayer(brId);
            for (size_t entryId = 0; entryId < entryLayer.GetCount(); ++entryId) {
                size_t entryRegId = entryLayer.GetKey(entryId);
                if (entryRegId == regId) {
                    Layer = entryLayer.GetSubLayer(entryId);
                    break;
                } else if (entryRegId > regId) {
                    break;
                }
            }
        }
    }

    inline void Init(const TArray4DPoly& array4D, ui32 docId, size_t brId, size_t regId) noexcept {
        if (docId < array4D.GetCount()) {
            const TArray4DPoly::TElementsLayer elementsLayer = array4D.GetSubLayer(docId);
            Init(elementsLayer, brId, regId);
        }
    }

    inline void Init(const TMemory4DArray& array4D, ui32 docId, size_t brId, size_t regId) noexcept {
        if (docId < array4D.Size()) {
            const TArray4DPoly::TElementsLayer elements = array4D.GetRow(docId);
            Init(elements, brId, regId);
        }
    }

    template<class T>
    inline const T& GetAsImpl(size_t pos) const noexcept {
        Y_ASSERT(pos <= GetCount());
        if (LegacyXmapInfo)
            return *reinterpret_cast<const T*>(LegacyXmapInfo);
        TArray4DPoly::TData data = Layer.GetData(pos);
        Y_ASSERT(data.Length >= sizeof(T));
        return *reinterpret_cast<const T*>(data.Start);
    }

public:
    TXMapWrapper(const TArray4DPoly& array4D, ui32 docId, size_t brId) noexcept
        : LegacyXmapInfo(nullptr)
    {
        // I expect hard inline and optimization in this place
        Init(array4D, docId, brId, 0);
    }

    TXMapWrapper(const TArray4DPoly& array4D, ui32 docId, size_t brId, size_t regId) noexcept
        : LegacyXmapInfo(nullptr)
    {
        // I expect hard inline and optimization in this place
        Init(array4D, docId, brId, regId);
    }

    TXMapWrapper(const TMemory4DArray& array4D, ui32 docId, size_t brId) noexcept
        : LegacyXmapInfo(nullptr)
    {
        // I expect hard inline and optimization in this place
        Init(array4D, docId, brId, 0);
    }

    TXMapWrapper(const TMemory4DArray& array4D, ui32 docId, size_t brId, size_t regId) noexcept
        : LegacyXmapInfo(nullptr)
    {
        // I expect hard inline and optimization in this place
        Init(array4D, docId, brId, regId);
    }


    TXMapWrapper(const TXMapInfo* data = nullptr) noexcept
        : LegacyXmapInfo(data)
    { }

    inline size_t GetCount() const noexcept {
        if (LegacyXmapInfo) {
            return 1;
        }
        return Layer.GetCount();
    }

    inline EEntryType GetType(size_t pos) const noexcept {
        if (LegacyXmapInfo) {
            static const EEntryType remap[] = {
                ET_CATALOG,
                ET_LINK,
            };
            Y_ASSERT(
                LegacyXmapInfo->EntryType == TXMapInfo::ET_LINK ||
                LegacyXmapInfo->EntryType == TXMapInfo::ET_CATALOG
            );
            return remap[LegacyXmapInfo->EntryType];
        }
        return EEntryType(Layer.GetKey(pos));
    }

    // Calling with pos greater or equal to GetCount is undefined behaviour
    template<class T>
    const T& GetAs(size_t pos) const noexcept {
        Y_ASSERT(GetType(pos) == T::TypeId);
        return GetAsImpl<T>(pos);
    }

    inline const TXMapInfo& GetLegacy(size_t pos) const noexcept {
        Y_ASSERT(GetType(pos) == ET_LINK || GetType(pos) == ET_CATALOG);
        return GetAsImpl<TXMapInfo>(pos);
    }
};
