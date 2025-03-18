#pragma once

#include "memory4d_poly.h"

class TArray4DPolyWrapper: public TArray4DPoly {
public:
    TArray4DPolyWrapper(const TMemory4DArray* memArray = nullptr)
        : MemArray(memArray)
    {
    }
    TArray4DPolyWrapper(const TString& fileName, const TTypeInfo& requiredTypes, bool isPolite = false, bool quiet = false, bool memLock = false) {
        Load(fileName, requiredTypes, isPolite, quiet, memLock);
    }

    size_t GetCount() const noexcept {
        return MemArray ? MemArray->Size() : TArray4DPoly::GetCount();
    }
    TElementsLayer GetSubLayer(size_t pos) const noexcept {
        return MemArray ? MemArray->GetRow(pos) : TArray4DPoly::GetSubLayer(pos);
    }

private:
    const TMemory4DArray* const MemArray = nullptr;
};
