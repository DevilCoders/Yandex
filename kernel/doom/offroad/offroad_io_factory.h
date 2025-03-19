#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/memory/blob.h>


namespace NDoom {


struct TOffreadReaderInputs {
    THolder<IInputStream> InfoStream;
    THolder<IInputStream> HitModelStream;
    THolder<IInputStream> KeyModelStream;
    TBlob HitSource;
    TBlob KeySource;
    TBlob FatSource;
    TBlob FatSubSource;
};

struct TOffroadWriterOutputs {
    THolder<IOutputStream> InfoStream;
    THolder<IOutputStream> HitModelStream;
    THolder<IOutputStream> KeyModelStream;
    THolder<IOutputStream> HitStream;
    THolder<IOutputStream> KeyStream;
    THolder<IOutputStream> FatStream;
    THolder<IOutputStream> FatSubStream;

    void Finish() {
        InfoStream->Finish();
        HitModelStream->Finish();
        KeyModelStream->Finish();
        HitStream->Finish();
        KeyStream->Finish();
        FatStream->Finish();
        FatSubStream->Finish();
    }
};


struct TOffroadIoFactory {
    static TVector<TString> GetIndexFiles(TString path);
    static TOffreadReaderInputs* OpenReaderInputs(TString path, bool lockMemory = false);
    static TOffroadWriterOutputs* OpenWriterOutputs(TString path);

    template<class Index>
    static typename Index::TKeyTable LoadKeyTable(Index*, IInputStream* input) {
        typename Index::TKeyModel model;
        model.Load(input);
        return new typename Index::TKeyTable(model);
    }

    template<class Index>
    static typename Index::TKeyTable LoadHitTable(Index*, IInputStream* input) {
        typename Index::TKeyModel model;
        model.Load(input);
        return new typename Index::TKeyTable(model);
    }
};


} // namespace NDoom
