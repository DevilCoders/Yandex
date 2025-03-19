#pragma once

#include <library/cpp/on_disk/2d_array/array2d.h>

#include <library/cpp/on_disk/head_ar/head_ar_2d.h>

#include "array_wrappers.h"

#include "dmap.h"
#include "xmap.h"
#include "xmap_wrapper.h"

typedef TFileMappedArrayWrapper<TDMapBlock> TXRefMappedArray; // Массив атрибутов документов для xref
typedef TFileMappedArrayWrapper<float> SearchAncWrdWht;

class TXRefMapped2DArray {
private:
    TArrayWithHead2D<TXMapInfo> TXRefMapped2DArrayImpl;
    TFileMapped2DArrayWrapper<ui32, TXMapInfo> TXRefMapped2DArrayLegacyImpl;
    bool LegacyMode;
    bool Inited;

public:
    typedef ui32 TIndex;
    typedef TXMapInfo TValue;

    TXRefMapped2DArray(const TMemory2DArray<TXMapInfo>* memory2DArray);
    TXRefMapped2DArray(const TString& name, bool isPolite, bool lazyLoad = false);
    TXRefMapped2DArray();
    bool Init(const TString& name, bool isPolite, bool lazyLoad = false);
    bool Init(const TMemoryMap& mapping, bool isPolite, bool lazyLoad = false);
    bool IsInited() const;

    size_t Size() const {
        if (Y_UNLIKELY(!IsInited()))
            return 0;
        return (LegacyMode) ? TXRefMapped2DArrayLegacyImpl.Size() : TXRefMapped2DArrayImpl.GetSize();
    }

    const TXMapInfo& GetAt(ui32 docId, size_t brId) const {
        return (LegacyMode) ? TXRefMapped2DArrayLegacyImpl.GetAt(docId, brId) : TXRefMapped2DArrayImpl.Get(docId, brId);
    }

    size_t GetLength(ui32 docId) const {
        return (LegacyMode) ? TXRefMapped2DArrayLegacyImpl.GetLength(docId) : TXRefMapped2DArrayImpl.GetRowLength(docId);
    }

    void SetSequential() {
        TXRefMapped2DArrayImpl.SetSequential();
    }

    void Evict() {
        TXRefMapped2DArrayImpl.Evict();
    }

private:
    template <typename T>
    bool DoInit(const T& initializer, bool isPolite, bool lazyLoad = false);
};

class TXRefMapped2DArrayPoly {
private:
    bool PolyMode;
    TArray4DPoly Array4DPolyImpl;
    TXRefMapped2DArray LegacyArrayImpl;
    const TMemory4DArray* Memory4DArray;

public:
    TXRefMapped2DArrayPoly(const TMemory4DArray* memory4DArray);
    TXRefMapped2DArrayPoly(const TString& name, bool isPolite);
    TXRefMapped2DArrayPoly();
    bool Init(const TString& name, bool isPolite);
    bool Init(const TMemoryMap& mapping, bool isPolite);
    bool IsInited() const;

    size_t Size() const {
        if (PolyMode) {
            return Array4DPolyImpl.GetCount();
        }
        if (Memory4DArray) {
            return Memory4DArray->Size();
        }
        return LegacyArrayImpl.Size();
    }

    TXMapWrapper GetAt(ui32 docId, size_t brId) const {
        if (PolyMode) {
            return TXMapWrapper(Array4DPolyImpl, docId, brId, 0);
        }
        if (Memory4DArray) {
            return TXMapWrapper(*Memory4DArray, docId, brId, 0);
        }
        return TXMapWrapper(&LegacyArrayImpl.GetAt(docId, brId));
    }

    size_t GetLength(ui32 docId) const {
        if (PolyMode) {
            return docId < Array4DPolyImpl.GetCount() ? Array4DPolyImpl.GetSubLayer(docId).GetCount() : 0;
        }
        if (Memory4DArray) {
            return docId < Memory4DArray->Size() ? Memory4DArray->GetLength(docId) : 0;
        }
        return LegacyArrayImpl.GetLength(docId);
    }

private:
    template <typename T>
    bool TryInitPoly(const T& initializer, bool isPolite);
};

const TArray4DPoly::TTypeInfo& GetTypeSizesInfo();

typedef TFileMapped2DArrayWrapper<ui32, ui8> TLerfMapped2DArray; // Двумерный массив атрибутов ссылок для lerf

typedef TMemory2DArray<std::pair<ui16, ui16>, /*useCMapFormat*/true> TMemorySearchAnchorData2;

typedef IMemoryArray<float> IMemorySearchAncWrdWht;
typedef IMemoryArray<TDMapBlock> IMemoryXRefMappedArray;
typedef TMemory4DArray TMemoryXRefMapped4DArrayPoly;
typedef TMemory2DArray<ui8> TMemoryLerfMapped2DArray;
