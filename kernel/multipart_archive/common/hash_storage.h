#pragma once

#include "mapping.h"
#include "hash.h"

#include <library/cpp/logger/global/global.h>

template<class T>
class TMappedHash : public IHashDataStorage<T> {
public:
    TMappedHash(const TFsPath& path, bool readOnly = false)
        : FileMap(path, readOnly ? RdOnly : OpenAlways)
        , Data(FileMap.GetPtr<T>())
        , ElementsCount(FileMap.Size() / sizeof(T))
    {
        CHECK_WITH_LOG(ElementsCount * sizeof(T) == FileMap.Size());
    }

    ui64 Size() const override {
        return ElementsCount;
    }

    void Resize(ui64 size) override {
        FileMap.Resize(size * sizeof(T));
        Data = FileMap.GetPtr<T>();
        ElementsCount = FileMap.Size() / sizeof(T);
    }

    T& GetData(ui64 index) override {
        CHECK_WITH_LOG(index < Size());
        return Data[index];
    }

    const T& GetData(ui64 index) const override {
        CHECK_WITH_LOG(index < Size());
        return Data[index];
    }

    void Init(ui64 bucketsCount) override {
        Resize(bucketsCount);
        TVector<T> tmp(bucketsCount, T());
        memcpy(Data, tmp.data(), tmp.size() * sizeof(T));
    }

private:
    TResizableMappedFile FileMap;
    T* Data;
    ui64 ElementsCount;
};
