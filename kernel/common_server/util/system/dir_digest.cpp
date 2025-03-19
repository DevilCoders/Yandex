#include "dir_digest.h"
#include <library/cpp/logger/global/global.h>

#include <library/cpp/digest/md5/md5.h>
#include <util/folder/iterator.h>

#include <sys/stat.h>
#include <util/folder/path.h>
#include <util/system/file.h>
#include <util/string/cast.h>

void FillDirHashes(TDirHashInfo& result, const TString& prefix, TFsPath configPath, const TRegExMatch* filter, const TRegExMatch* exclude) {
    if (configPath.IsDirectory()) {
        TVector<TFsPath> children;
        configPath.List(children);
        for (TVector<TFsPath>::const_iterator i = children.begin(); i != children.end(); ++i)
            FillDirHashes(result, (!!prefix ? prefix + "/" : TString()) + i->GetName(), *i, filter, exclude);
    }
    else if (
        (!filter || filter->Match(configPath.GetName().data())) &&
        (!exclude || !exclude->Match(configPath.GetName().data()))
        )
    {
        auto& info = result[!!prefix ? prefix : configPath.GetName()];
        info.Hash =  MD5::File(configPath.GetPath().data());
        info.Size = TFile(configPath.GetPath(), RdOnly).GetLength();
        DEBUG_LOG << "file info for " << configPath.GetPath() << " : " << info.Hash << "/" << info.Size << Endl;
    }
}

TDirHashInfo GetDirHashes(const TString& path, const TRegExMatch* filter, const TRegExMatch* exclude) {
    TDirHashInfo result;
    FillDirHashes(result, TString(), path, filter, exclude);
    return result;
}
