#pragma once

#include "array4d_poly.h"

#include <util/generic/buffer.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

class TMemory4DArray: private TNonCopyable {
private:
    TVector<TBuffer> Rows;
    TVector<char> LayerJumps;

public:
    TMemory4DArray(size_t maxSize)
        : Rows(maxSize)
        , LayerJumps(maxSize)
    {
    }

    TArray4DPoly::TElementsLayer GetRow(size_t pos1) const {
        Y_ASSERT(pos1 < Size());
        return TArray4DPoly::TElementsLayer(Rows[pos1], LayerJumps[pos1]);
    }

    size_t GetLength(size_t pos1) const {
        return GetRow(pos1).GetCount();
    }

    size_t Size() const {
        return Rows.size();
    }

    void SetRow(size_t pos, TBuffer& row, char layerJump) {
        Rows[pos].Swap(row);
        LayerJumps[pos] = layerJump;
    }

    void RemoveRow(size_t pos) {
        TBuffer tmpBuf;
        SetRow(pos, tmpBuf, /*layerJump=*/0);
    }
};
