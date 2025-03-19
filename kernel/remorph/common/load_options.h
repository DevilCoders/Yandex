#pragma once

/// @file load_options.h Опции загрузки.

#include <kernel/gazetteer/gazetteer.h>

#include <util/folder/path.h>
#include <util/generic/string.h>

namespace NRemorph {

struct TFileLoadOptions {
    const NGzt::TGazetteer* Gazetteer;
    TString FilePath;
    TString BaseDirPath;

    explicit TFileLoadOptions(const TString& filePath, const NGzt::TGazetteer* gazetteer = nullptr, const TString& baseDirPath = TString())
        : Gazetteer(gazetteer)
        , FilePath(filePath)
        , BaseDirPath()
    {
        SetBaseDirPath(baseDirPath);
    }

    inline void SetBaseDirPath(const TString& baseDirPath) {
        if (baseDirPath.empty()) {
            BaseDirPath = TFsPath(FilePath).Parent().RealPath().c_str();
            return;
        }

        BaseDirPath = TFsPath(baseDirPath).RealPath().c_str();
    }
};

struct TStreamLoadOptions {
    const NGzt::TGazetteer* Gazetteer;
    IInputStream& Input;

    explicit TStreamLoadOptions(IInputStream& input)
        : Gazetteer(nullptr)
        , Input(input)
    {
    }

    explicit TStreamLoadOptions(const NGzt::TGazetteer* gazetteer, IInputStream& input)
        : Gazetteer(gazetteer)
        , Input(input)
    {
    }
};

} // NRemorph
