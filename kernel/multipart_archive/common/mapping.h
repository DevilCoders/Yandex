#pragma once

#include <library/cpp/logger/global/global.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/system/fs.h>


class TResizableMappedFile {
public:
    TResizableMappedFile(const TFsPath& path, EOpenMode mode) {
        MappingMode = mode == RdOnly ? TMemoryMapCommon::oRdOnly : TMemoryMapCommon::oRdWr;
        File.Reset(new TFile(path.GetPath(), mode));
        FileMap.Reset(new TFileMap(*File, MappingMode));
        FileMap->Map(0, File->GetLength());
    }

    virtual ~TResizableMappedFile() {}

    void Resize(size_t newSize) {
        CHECK_WITH_LOG(FileMap);
        FileMap->Flush();
        size_t oldSize = FileMap->Length();
        FileMap.Reset(nullptr);
        File->Resize(newSize);
        FileMap.Reset(new TFileMap(*File.Get(), MappingMode));
        FileMap->Map(0, File->GetLength());
        DoResize(oldSize, newSize);
    }


    virtual void DoResize(size_t, size_t) {}

    template<class T>
    T* GetPtr() {
        CHECK_WITH_LOG(FileMap);
        return static_cast<T*>(FileMap->Ptr());
    }

    template<class T>
    const T* GetPtr() const {
        CHECK_WITH_LOG(FileMap);
        return static_cast<T*>(FileMap->Ptr());
    }

    size_t Size() const {
        CHECK_WITH_LOG(FileMap);
        return FileMap->Length();
    }

private:
    THolder<TFile> File;
    THolder<TFileMap> FileMap;
    TMemoryMapCommon::EOpenMode MappingMode;
};
