#pragma once

#include "simple_module.h"

#include <util/stream/file.h>

template <class T>
class TFileModule: public TSimpleModule {
private:
    const TString FileName;
    const EOpenMode FileOpenMode;

protected:
    THolder<T> FileStream;

    TFileModule(const TString& moduleName, const TString& name, EOpenMode fileOpenMode)
        : TSimpleModule(moduleName)
        , FileName(name)
        , FileOpenMode(fileOpenMode)
    {
        Bind(this).template To<&TFileModule::Start>("start");
    }

private:
    void Start() {
        TFile file(FileName, FileOpenMode);
        FileStream.Reset(new T(file));
    }
};
