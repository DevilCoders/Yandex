#pragma once

#include "memory_array.h"

#include <library/cpp/on_disk/2d_array/array2d.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/system/filemap.h>

template<class T>
class TFileMappedArrayWrapper : private TNonCopyable {
private:
    THolder<TFileMappedArray<T> > MappedArray;
    const IMemoryArray<T>* MemoryArray; // Someone other holds it.
    bool FileBased;
public:
    TFileMappedArrayWrapper()
        : MappedArray(new TFileMappedArray<T>)
        , MemoryArray(nullptr) // Fool proof action.
        , FileBased(true)
    { }
    TFileMappedArrayWrapper(const IMemoryArray<T>* memoryArray)
        : MemoryArray(memoryArray)
        , FileBased(false)
    { }
    inline void Init(const TString& name) {
        Y_ASSERT(FileBased);
        MappedArray->Init(name);
    }
    inline void Init(const TMemoryMap& mapping) {
        Y_ASSERT(FileBased);
        MappedArray->Init(mapping);
    }
    const T &operator[](size_t pos) const {
        return (FileBased) ? (*MappedArray)[pos] : (*MemoryArray)[pos];
    }
    size_t Size() const {
        return (FileBased) ? MappedArray->size() : MemoryArray->Size();
    }
};

template<class I, class T, bool useCMapFormat = false>
class TFileMapped2DArrayWrapper : private TNonCopyable {
public:
    typedef I TIndex;
    typedef T TValue;

private:
    THolder<FileMapped2DArray<I, T> > Mapped2DArray;
    const TMemory2DArray<T, useCMapFormat>* Memory2DArray; // Someone other holds it.
    bool FileBased;
public:
    TFileMapped2DArrayWrapper()
        : Mapped2DArray(new FileMapped2DArray<I, T>)
        , Memory2DArray(nullptr) // Fool proof action.
        , FileBased(true)
    { }
    explicit TFileMapped2DArrayWrapper(const TString& name)
        : Mapped2DArray(new FileMapped2DArray<I, T>(name))
        , Memory2DArray(NULL) // Fool proof action.
        , FileBased(true)
    { }
    explicit TFileMapped2DArrayWrapper(const TMemory2DArray<T, useCMapFormat>* memory2DArray)
        : Memory2DArray(memory2DArray)
        , FileBased(false)
    { }
    inline bool Init(const TString& name) {
        Y_ASSERT(FileBased);
        return Mapped2DArray->init(name);
    }
    inline bool Init(const TMemoryMap& mapping) {
        Y_ASSERT(FileBased);
        return Mapped2DArray->Init(mapping);
    }
    const T *operator[](size_t pos) const {
        return (FileBased) ? (*Mapped2DArray)[pos] : (*Memory2DArray)[pos];
    }
    size_t Size() const {
        return (FileBased) ? Mapped2DArray->size() : Memory2DArray->Size();
    }
    const T &GetAt(size_t pos1, size_t pos2) const {
        return (FileBased) ? Mapped2DArray->GetAt(pos1, pos2) : Memory2DArray->GetAt(pos1, pos2);
    }
    size_t GetLength(size_t pos) const {
        return (FileBased) ? Mapped2DArray->GetLength(pos) : Memory2DArray->GetLength(pos);
    }
    const T *GetBegin(size_t pos) const {
        return (FileBased) ? Mapped2DArray->GetBegin(pos) : (*Memory2DArray)[pos];
    }
    const T *GetEnd(size_t pos) const {
        return (FileBased) ? Mapped2DArray->GetEnd(pos) : GetBegin(pos) + GetLength(pos);
    }
};
