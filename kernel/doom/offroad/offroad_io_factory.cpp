#include "offroad_io_factory.h"

#include <util/stream/file.h>

namespace NDoom {

TVector<TString> TOffroadIoFactory::GetIndexFiles(TString path) {
    if (path.EndsWith('.'))
        path.pop_back();

    TVector<TString> result;
    result.push_back(path + ".info");
    result.push_back(path + ".inv.model");
    result.push_back(path + ".key.model");
    result.push_back(path + ".inv");
    result.push_back(path + ".key");
    result.push_back(path + ".fat");
    result.push_back(path + ".fat.idx");
    return result;
}

TOffreadReaderInputs* TOffroadIoFactory::OpenReaderInputs(TString path, bool lockMemory) {
    THolder<TOffreadReaderInputs> result(new TOffreadReaderInputs());

    if (path.EndsWith('.'))
        path.pop_back();

    result->InfoStream.Reset(new TIFStream(path + ".info"));
    result->HitModelStream.Reset(new TIFStream(path + ".inv.model"));
    result->KeyModelStream.Reset(new TIFStream(path + ".key.model"));

    if (lockMemory) {
        result->HitSource = TBlob::LockedFromFile(path + ".inv");
        result->KeySource = TBlob::LockedFromFile(path + ".key");
        result->FatSource = TBlob::LockedFromFile(path + ".fat");
        result->FatSubSource = TBlob::LockedFromFile(path + ".fat.idx");
    } else {
        result->HitSource = TBlob::FromFile(path + ".inv");
        result->KeySource = TBlob::FromFile(path + ".key");
        result->FatSource = TBlob::FromFile(path + ".fat");
        result->FatSubSource = TBlob::FromFile(path + ".fat.idx");
    }

    return result.Release();
}

TOffroadWriterOutputs* TOffroadIoFactory::OpenWriterOutputs(TString path) {
    THolder<TOffroadWriterOutputs> result(new TOffroadWriterOutputs());

    if (path.EndsWith('.'))
        path.pop_back();

    result->InfoStream.Reset(new TOFStream(path + ".info"));
    result->HitModelStream.Reset(new TOFStream(path + ".inv.model"));
    result->KeyModelStream.Reset(new TOFStream(path + ".key.model"));
    result->HitStream.Reset(new TOFStream(path + ".inv"));
    result->KeyStream.Reset(new TOFStream(path + ".key"));
    result->FatStream.Reset(new TOFStream(path + ".fat"));
    result->FatSubStream.Reset(new TOFStream(path + ".fat.idx"));

    return result.Release();
}


} // namespace NDoom
