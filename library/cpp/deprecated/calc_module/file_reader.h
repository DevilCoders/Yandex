#pragma once

#include "file_module.h"

class TFileReader: public TFileModule<TIFStream> {
private:
    typedef TFileModule<TIFStream> TBase;

public:
    TFileReader(const TString& name)
        : TBase("TFileReader", name, RdOnly | Seq)
    {
        Bind(this).To<void*, size_t, bool&, &TFileReader::Read>("output");
        Bind(this).To<&TFileReader::Finish>("finish");
    }

private:
    void Read(void* buf, size_t len, bool& result) {
        result = FileStream->Load(buf, len) == len;
    }
    void Finish() {
        FileStream.Destroy();
    }
};
