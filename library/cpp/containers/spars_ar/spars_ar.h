#pragma once

#include <library/cpp/deprecated/mapped_file/mapped_file.h>

#include <util/system/file.h>
#include <util/system/filemap.h>

#include <cstddef>
#include <cstring>

template <class TArrayEl>
class TSparsedArray {
private:
    int Shift;
    unsigned Size;
    TArrayEl** TArrayTable;
    TMappedFile Map;
    TArrayEl dummy;

public:
    using value_type = TArrayEl;

    TSparsedArray(int numBitsInSize);
    ~TSparsedArray();

    void Flush();
    inline void PutAt(unsigned long docHandle, const TArrayEl&);
    const TArrayEl& GetAt(unsigned long docHandle) const;

    int Load(const char* name);
    int Save(TFile& F);
    size_t SaveSize() const;
    unsigned RealSize;
};

template <class TArrayEl>
TSparsedArray<TArrayEl>::TSparsedArray(int numBitsInSize) {
    Shift = numBitsInSize / 2;
    Size = 1 << Shift;
    TArrayTable = new TArrayEl*[Size];
    for (size_t i = 0; i < Size; i++)
        TArrayTable[i] = nullptr;
    memset(&dummy, 0, sizeof(dummy));
    RealSize = 0;
}

template <class TArrayEl>
inline void TSparsedArray<TArrayEl>::PutAt(unsigned long docHandle, const TArrayEl& el) {
    unsigned int plane = (unsigned int)(docHandle >> Shift);
    if (plane < Size) {
        unsigned int pos = (unsigned int)(docHandle & (Size - 1));
        if (TArrayTable[plane] == nullptr) {
            TArrayTable[plane] = new TArrayEl[Size];
            memset(TArrayTable[plane], 0, Size * sizeof(TArrayEl));
        }
        TArrayTable[plane][pos] = el;
    }
}

template <class TArrayEl>
const TArrayEl& TSparsedArray<TArrayEl>::GetAt(unsigned long docHandle) const {
    unsigned int plane = (unsigned int)(docHandle >> Shift);
    unsigned int pos = (unsigned int)(docHandle & (Size - 1));
    if (plane >= Size || TArrayTable[plane] == nullptr)
        return dummy;
    return TArrayTable[plane][pos];
}

template <class TArrayEl>
TSparsedArray<TArrayEl>::~TSparsedArray() {
    Flush();
    delete[] TArrayTable;
}

template <class TArrayEl>
void TSparsedArray<TArrayEl>::Flush() {
    for (size_t i = 0; i < Size; i++)
        if (TArrayTable[i]) {
            if (Map.getData() == nullptr)
                delete[] TArrayTable[i];
            TArrayTable[i] = nullptr;
        }
    Map.term();
}

template <class TArrayEl>
int TSparsedArray<TArrayEl>::Load(const char* name) {
    try {
        Map.init(name);
    } catch (...) {
        return 0;
    }
    size_t L = Map.getSize();
    if (L == 0)
        return 0;
    unsigned i;
    for (i = 0; i <= (L - 1) / (sizeof(TArrayEl) * Size); i++)
        TArrayTable[i] = (TArrayEl*)(Map.getData()) + i * Size;

    for (; i < Size; i++)
        TArrayTable[i] = nullptr;

    RealSize = (unsigned)(L / sizeof(TArrayEl));

    return 1;
}

template <class TArrayEl>
size_t TSparsedArray<TArrayEl>::SaveSize() const {
    unsigned MaxSize = 0;
    unsigned i;
    for (i = 0; i < Size; i++) {
        if (TArrayTable[i])
            MaxSize = i + 1;
    }
    return MaxSize * Size;
}

template <class TArrayEl>
int TSparsedArray<TArrayEl>::Save(TFile& F) {
    unsigned MaxSize = 0;
    unsigned i;
    TArrayEl* DummyArray = nullptr;
    for (i = 0; i < Size; i++) {
        if (TArrayTable[i])
            MaxSize = i + 1;
    }
    for (i = 0; i < MaxSize; i++) {
        if (TArrayTable[i])
            F.Write(TArrayTable[i], Size * sizeof(TArrayEl));
        else {
            if (DummyArray == nullptr) {
                DummyArray = new TArrayEl[Size];
                memset(DummyArray, 0, Size * sizeof(TArrayEl));
            }
            F.Write(DummyArray, Size * sizeof(TArrayEl));
        }
    }
    delete[] DummyArray;
    return 0;
}
