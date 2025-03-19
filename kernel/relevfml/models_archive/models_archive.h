#pragma once

#include <kernel/matrixnet/mn_sse.h>

#include <library/cpp/archive/models_archive_reader.h>
#include <library/cpp/archive/yarchive.h>

#include <util/generic/map.h>
#include <functional>
#include <util/memory/blob.h>
#include <util/folder/path.h>
#include <util/system/filemap.h>

using TModels = TMap<TString, NMatrixnet::TMnSsePtr>;

namespace NModelsArchive {

// load archive from memory (copy data if archive is compressed)
// from specific directory (not recursive) (default is root)

void LoadModels(const IModelsArchiveReader& reader,
                TModels& result,
                const TStringBuf& source = "archive",
                const TStringBuf& directory = "",
                const bool needCopy = false);

void LoadModels(const TBlob& blob,
                TModels& result,
                const TStringBuf& source = "blob",
                const TStringBuf& directory = "",
                const bool needCopy = false);

void LoadModels(const TFileMap& fileMap,
                TModels& result,
                const TStringBuf& source = "filemap",
                const TStringBuf& directory = "",
                const bool needCopy = false);

inline TFsPath ArchiveInternalPath(const TStringBuf& path) {
    if (path.empty() || path == ".") {
        return TFsPath("/");
    }

    // all files in archive's root dir have leading slash ("/mxnet.info")
    if (path[0] != '/') {
        return TFsPath("/") / path;
    }

    return TFsPath(path);
}

// load custom objects from archive
template<typename TLoader>
void Load(const IModelsArchiveReader& reader, TLoader&& loader, const TStringBuf& directory) {
    const TFsPath path = ArchiveInternalPath(directory);
    for (size_t i = 0, iEnd = reader.Count(); i < iEnd; ++i) {
        const TString key = reader.KeyByIndex(i);
        const TFsPath parent = TFsPath(key).Parent();
        if (parent == path) {
            try {
                loader(reader, key);
            } catch(yexception &e) {
                ythrow e << "Can't load " << key.Quote() << " from models archive: " << e.what();
            }
        }
    }
}

template<typename TLoader>
void Load(const TBlob& blob, TLoader&& loader, const TStringBuf& directory) {
    const TArchiveReader reader(blob);
    Load(reader, std::forward<TLoader>(loader), directory);
}

template<typename TLoader>
void Load(const TFileMap& fileMap, TLoader&& loader, const TStringBuf& directory) {
    const TBlob blob = TBlob::NoCopy(fileMap.Ptr(), fileMap.MappedSize());
    Load(blob, std::forward<TLoader>(loader), directory);
}

/**
 * JSON metainfo about models archive (see SEARCH-1360)
 * @return true when metainfo was found, false otherwise
 **/
bool LoadMetaInfo(const IModelsArchiveReader& reader, TString& metaInfo);

template <typename TGenericMn>
void WriteMatrixnetInfo(TGenericMn& mxnet, const TStringBuf name, const TStringBuf& source) {
    auto info = mxnet.GetInfo();
    if (!info->contains("formula-id") || info->at("formula-id").empty()) {
        mxnet.SetInfo("formula-id", "md5-" + mxnet.MD5());
    }

    mxnet.SetInfo("fname", TString(name));
    mxnet.SetInfo("dynamic_source", TString("archive: ") + source);
}

} // namespace NModelsArchive


// for backward compatibility
void LoadModelsFromArchive(const TBlob& blob,
                           TModels& result,
                           const TStringBuf& source = "blob",
                           const TStringBuf& directory = "",
                           const bool needCopy = false);
