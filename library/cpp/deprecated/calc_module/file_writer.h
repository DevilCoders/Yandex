#pragma once

#include "file_module.h"

class TFileWriter: public TFileModule<TOFStream> {
private:
    typedef TFileModule<TOFStream> TBase;

public:
    TFileWriter(const TString& name)
        : TBase("TFileWriter", name, WrOnly | CreateAlways | Seq)
    {
        Bind(this).To<const void*, size_t, &TFileWriter::Write>("input");
        Bind(this).To<&TFileWriter::Finish>("finish");
    }

private:
    void Write(const void* buf, size_t len) {
        FileStream->Write(buf, len);
    }
    void Finish() {
        FileStream->Flush();
        FileStream.Destroy();
    }
};
