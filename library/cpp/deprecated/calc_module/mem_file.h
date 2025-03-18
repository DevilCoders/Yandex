#pragma once

#include "simple_module.h"

#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>

class TMemFile: public TSimpleModule {
private:
    TBuffer Buffer;
    THolder<TBufferStream> BufferStream;

    TMemFile()
        : TSimpleModule("TMemFile")
    {
        Bind(this).To<const void*, size_t, &TMemFile::Write>("input");
        Bind(this).To<void*, size_t, bool&, &TMemFile::Read>("output");
        Bind(this).To<TBlob, &TMemFile::GetBlob>("get_blob");
        Bind(this).To<&TMemFile::Start>("start,reset");
        Bind(this).To<&TMemFile::Finish>("finish");
    }

public:
    static TCalcModuleHolder BuildModule() {
        return new TMemFile;
    }

private:
    void Start() {
        BufferStream.Reset(new TBufferStream(Buffer));
    }
    void Write(const void* buf, size_t len) {
        Y_ASSERT(!!BufferStream);
        BufferStream->Write(buf, len);
    }
    TBlob GetBlob() {
        return TBlob::NoCopy(Buffer.data(), Buffer.size());
    }
    void Read(void* buf, size_t len, bool& res) {
        Y_ASSERT(!!BufferStream);
        res = BufferStream->Read(buf, len) == len;
    }
    void Finish() {
        BufferStream.Destroy();
        Buffer.Reset();
    }
};
