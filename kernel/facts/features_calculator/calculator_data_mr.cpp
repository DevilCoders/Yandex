#include "calculator_data_mr.h"

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/folder/path.h>
#include <util/memory/blob.h>
#include <util/generic/hash_set.h>
#include <util/system/filemap.h>
#include <util/system/fstat.h>

namespace {
    const TString CALCULATOR_DATA_MR_ARCHIVE_FILE_NAME = ".calculator_data.archive";

    ui64 MakeArchive(const TString& pathPrefix, const TVector<TString>& fileNames, const TString& archiveFileName) {
        THolder<IOutputStream> archive = OpenOutput(archiveFileName);
        TArchiveWriter w(archive.Get());
        ui64 totalSize = 0;
        for (const TString& fileName : fileNames) {
            totalSize += GetFileLength(fileName);
            TMappedFileInput in(fileName);
            w.Add(TFsPath(fileName).RelativeTo(pathPrefix), &in);
        }
        w.Finish();
        return totalSize;
    }
}

namespace NUnstructuredFeatures {

    NYT::TUserJobSpec* MakeCalculatorDataJobSpec(const TConfig& config, bool addKnnModel, const TVector<TString>& specificConfigFiles) {
        TVector<TString> fileNames;
        if (specificConfigFiles.empty()) {
            fileNames = config.GetUsedFiles();
        } else {
            for (const TString& configFile : specificConfigFiles) {
                fileNames.push_back(config.GetFilePath(configFile));
            }
        }
        if (addKnnModel) {
            TString knnModelDirName = config.GetPath("files/knn");
            TVector<TFsPath> knnFileNames;
            TFsPath(knnModelDirName).List(knnFileNames);
            for (const TString& fileName : knnFileNames) {
                fileNames.push_back(fileName);
            }
        }
        fileNames.push_back(config.GetConfigFileName());

        const TString archiveFileName = TFsPath(config.GetDataPath()) / CALCULATOR_DATA_MR_ARCHIVE_FILE_NAME;
        ui64 totalSize = MakeArchive(config.GetDataPath(), fileNames, archiveFileName);

        NYT::TUserJobSpec* jobSpec = new NYT::TUserJobSpec();
        jobSpec->AddLocalFile(archiveFileName);
        jobSpec->MemoryLimit(2 * totalSize);
        jobSpec->ExtraTmpfsSize(2 * totalSize);
        return jobSpec;
    }

    TConfig* InitMapReduceConfig(const TString& configFileName, size_t version) {
        TMemoryMap map(CALCULATOR_DATA_MR_ARCHIVE_FILE_NAME);
        TBlob blob(TBlob::FromMemoryMapSingleThreaded(map, 0, map.Length()));
        TArchiveReader reader(blob);
        const size_t count = reader.Count();
        for (size_t i = 0; i < count; ++i) {
            const TString fileName = reader.KeyByIndex(i);
            const TFsPath path(fileName);
            path.Parent().MkDirs();
            THolder<IInputStream> in = reader.ObjectByKey(fileName);
            TFixedBufferFileOutput out(path);
            TransferData(in.Get(), &out);
            out.Finish();
        }
        TFsPath(CALCULATOR_DATA_MR_ARCHIVE_FILE_NAME).ForceDelete();
        return new NUnstructuredFeatures::TConfig(".", configFileName, version);
    }
}
