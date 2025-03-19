#include <util/folder/dirut.h>

#include "xref_types.h"

TXRefMapped2DArray::TXRefMapped2DArray(const TMemory2DArray<TXMapInfo>* memory2DArray)
    : TXRefMapped2DArrayLegacyImpl(memory2DArray)
    , LegacyMode(true)
    , Inited(true)
{
}

TXRefMapped2DArray::TXRefMapped2DArray(const TString& name, bool isPolite, bool lazyLoad)
    : LegacyMode(false)
    , Inited(false)
{
    Init(name, isPolite, lazyLoad);
}

TXRefMapped2DArray::TXRefMapped2DArray()
    : LegacyMode(false)
    , Inited(false)
{
}

template <typename T>
bool TXRefMapped2DArray::DoInit(const T& initializer, bool isPolite, bool lazyLoad) {
    try {
        LegacyMode = false;
        TXRefMapped2DArrayImpl.Load(initializer, isPolite, /*quiet=*/false, lazyLoad);
        Inited = true;
        return true;
    } catch (const yexception&) {
        // most probably, obsolete file format, it should not be used now
        // approx 2 years passed since it was deprecated
        // (ask vvp@, sukhoi@, mvel@ and robot@ people for details)
        Cerr << "TXRefMapped2DArray warning: trying to switch to legacy mode due to error: " << CurrentExceptionMessage() << Endl;
    }
    LegacyMode = true;
    if (TXRefMapped2DArrayLegacyImpl.Init(initializer)) {
        Inited = true;
        return true;
    }
    return false;
}

bool TXRefMapped2DArray::Init(const TString& name, bool isPolite, bool lazyLoad) {
    if (!NFs::Exists(name))
        return false;

    return DoInit(name, isPolite, lazyLoad);
}

bool TXRefMapped2DArray::Init(const TMemoryMap& mapping, bool isPolite, bool lazyLoad) {
    return DoInit(mapping, isPolite, lazyLoad);
}

bool TXRefMapped2DArray::IsInited() const {
    return Inited;
}

TXRefMapped2DArrayPoly::TXRefMapped2DArrayPoly(const TMemory4DArray* memory4DArray)
    : PolyMode(false)
    , Memory4DArray(memory4DArray)
{
}

TXRefMapped2DArrayPoly::TXRefMapped2DArrayPoly(const TString& name, bool isPolite)
    : Memory4DArray(nullptr)
{
    Init(name, isPolite);
}

TXRefMapped2DArrayPoly::TXRefMapped2DArrayPoly()
    : PolyMode(false)
    , LegacyArrayImpl()
    , Memory4DArray(nullptr)
{
}


template <typename T>
bool TXRefMapped2DArrayPoly::TryInitPoly(const T& initializer, bool isPolite) {
    PolyMode = false;
    try {
        Array4DPolyImpl.Load(initializer, GetTypeSizesInfo(), isPolite);
        PolyMode = true;
        return true;
    } catch (const yexception&) {
        // Cerr << "TXRefMapped2DArrayPoly warning: can't load 4DArrayPoly, trying to switch to legacy mode due to error: " << CurrentExceptionMessage() << Endl;
        return false;
    }
}

bool TXRefMapped2DArrayPoly::Init(const TString& name, bool isPolite) {
    return TryInitPoly(name, isPolite) || LegacyArrayImpl.Init(name, isPolite);
}

bool TXRefMapped2DArrayPoly::Init(const TMemoryMap& mapping, bool isPolite) {
    return TryInitPoly(mapping, isPolite) || LegacyArrayImpl.Init(mapping, isPolite);
}

bool TXRefMapped2DArrayPoly::IsInited() const {
    return PolyMode || Memory4DArray || LegacyArrayImpl.IsInited();
}

const TArray4DPoly::TTypeInfo& GetTypeSizesInfo() {
    struct TTypeSizesInfoHolder {
        TArray4DPoly::TTypeInfo Info;

        TTypeSizesInfoHolder() {
            Info[TXMapEnums::ET_LINK] = sizeof(TLinkXMap);
            Info[TXMapEnums::ET_CATALOG] = sizeof(TCatalogXMap);
        }
    };

    const static TTypeSizesInfoHolder typeSizesInfoHolder;
    return typeSizesInfoHolder.Info;
}
