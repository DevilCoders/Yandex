#pragma once

#include <util/system/types.h>
#include <util/generic/ylimits.h>
#include <util/generic/string.h>

#define UNKNOWN_POSITION Max<ui64>()
#define UNKNOWN_HASH Max<ui64>()


template <class TValue>
class TCell {
private:
    ui64 Hash = UNKNOWN_HASH;
    ui64 Next = UNKNOWN_POSITION;
    TValue DataRef;

public:
    TCell() {
       memset(&DataRef, 0, sizeof(TValue));
    }

    TCell(ui64 hash, const TValue& data)
            : Hash(hash)
            , DataRef(data) {}

    const TValue& Data() const {
        return DataRef;
    }

    void UpdateData(const TValue& val) {
        DataRef = val;
    }

    ui64 GetHash() const {
        return Hash;
    }

    ui64 GetNext() const {
        return Next;
    }

    void SetNext(ui64 val) {
        Next = val;
    }

    bool operator==(const TCell& other) {
        return Hash == other.Hash;
    }

    inline explicit operator bool() const {
        return !IsNull() && IsInHash();
    }

    bool IsNull() const {
        TCell empty;
        memset(&empty, 0, sizeof(TCell));
        return memcmp(this, &empty, sizeof(TCell)) == 0;
    }

    bool IsInHash() const {
        return Next != UNKNOWN_POSITION;
    }

    ui64 RemoveFromHash() {
        ui64 result = Next;
        Next = UNKNOWN_POSITION;
        return result;
    }

    void Swap(TCell& cell) {
        TValue data = DataRef;
        ui64 hash = Hash;
        Hash = cell.Hash;
        DataRef = cell.DataRef;
        cell.DataRef = data;
        cell.Hash = hash;
    }
};
