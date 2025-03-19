#include "models_archive.h"

#include <kernel/matrixnet/mn_dynamic.h>

#include <util/ysaveload.h>

namespace {

class TMatrixnetLoader {
public:
    TMatrixnetLoader(const bool needCopy, const TStringBuf& source, TModels& result)
        : NeedCopy(needCopy)
        , Source(source)
        , Result(result)
    {
    }

    void operator()(const IModelsArchiveReader& reader, const TString& key) {
        if (key.EndsWith(".info")) {
            const TString name = NMatrixnet::ModelNameFromFilePath(key);
            const bool copy = NeedCopy || reader.Compressed();
            Result[name] = copy ? LoadDynamic(reader, key, name) : LoadStatic(reader, key, name);
        }
    }

private:
    NMatrixnet::TMnSsePtr LoadDynamic(const IModelsArchiveReader& reader,
                                      const TString& key,
                                      const TString& name) const {
        auto pmn = MakeHolder<NMatrixnet::TMnSseDynamic>();
        ::Load(reader.ObjectByKey(key).Get(), *pmn);
        NModelsArchive::WriteMatrixnetInfo(*pmn, name, Source);
        return NMatrixnet::TMnSsePtr(pmn.Release());
    }

    NMatrixnet::TMnSsePtr LoadStatic(const IModelsArchiveReader& reader,
                                     const TString& key,
                                     const TString& name) const {
        const TBlob blob = reader.BlobByKey(key);
        auto pmn = MakeHolder<NMatrixnet::TMnSseInfo>(blob.Data(), blob.Length());
        NModelsArchive::WriteMatrixnetInfo(*pmn, name, Source);
        return NMatrixnet::TMnSsePtr(pmn.Release());
    }

    const bool NeedCopy;
    const TStringBuf& Source;
    TModels& Result;
};

} // namespace

namespace NModelsArchive {

void LoadModels(const IModelsArchiveReader& reader,
                TModels& result,
                const TStringBuf& source,
                const TStringBuf& directory,
                const bool needCopy) {
    TMatrixnetLoader loader(needCopy, source, result);
    Load(reader, loader, directory);
}

void LoadModels(const TBlob& blob,
                TModels& result,
                const TStringBuf& source,
                const TStringBuf& directory,
                const bool needCopy) {
    TMatrixnetLoader loader(needCopy, source, result);
    Load(blob, loader, directory);
}

void LoadModels(const TFileMap& fileMap,
                TModels& result,
                const TStringBuf& source,
                const TStringBuf& directory,
                const bool needCopy) {
    TMatrixnetLoader loader(needCopy, source, result);
    Load(fileMap, loader, directory);
}

bool LoadMetaInfo(const IModelsArchiveReader& reader, TString& metaInfo) {
    metaInfo.clear();
    constexpr TStringBuf metaKey = "/meta";
    if (!reader.Has(metaKey)) {
        return false;
    }
    TBlob metaData = reader.BlobByKey(metaKey);
    metaInfo = TString(metaData.AsCharPtr(), metaData.Size());
    return true;
}

} // namespace NModelsArchive

void LoadModelsFromArchive(const TBlob& blob,
                           TMap<TString, NMatrixnet::TMnSsePtr> &result,
                           const TStringBuf& source,
                           const TStringBuf& directory,
                           const bool needCopy) {
    NModelsArchive::LoadModels(blob, result, source, directory, needCopy);
}
