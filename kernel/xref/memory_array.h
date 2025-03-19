#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <util/generic/cast.h>

template<class T>
class IMemoryArray {
public:
    virtual ~IMemoryArray() {
    }

    virtual const T& operator[](size_t pos) const = 0;
    virtual size_t Size() const = 0;
};

template<class T, bool useCMapFormat = false>
class TMemory2DArray : private TNonCopyable {
public:
    typedef TVector<T> TRow;
private:
    TVector<TRow> Array2D;
    static char Dummy[sizeof(T)];
public:
    TMemory2DArray(size_t maxSize)
        : Array2D(maxSize)
    {
        memset(Dummy, 0, sizeof Dummy);
    }
    const T* operator[](size_t pos) const {
        return (useCMapFormat) ? &Array2D[pos][0] - 1 : &Array2D[pos][0];
    }
    const T &GetAt(size_t pos1, size_t pos2) const {
        if (useCMapFormat)
            pos2--;
        if (Array2D[pos1].size() > pos2)
            return Array2D[pos1][pos2];
        return *reinterpret_cast<T*>(Dummy);
    }
    size_t GetLength(size_t pos) const {
        return Array2D[pos].size();
    }
    size_t Size() const {
        return Array2D.size();
    }
    void SetRow(size_t pos, TVector<T>& row) {
        Array2D[pos].swap(row);
    }
    void RemoveRow(size_t pos) {
        TVector<T>().swap(Array2D[pos]);
    }
};

template<class T, bool useCMapFormat> char TMemory2DArray<T, useCMapFormat>::Dummy[sizeof(T)];
