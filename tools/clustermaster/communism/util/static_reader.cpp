#include "static_reader.h"

#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/input.h>

TStaticReader::TStaticReader(const unsigned char* data, size_t dataSize)
    : TArchiveReader(TBlob::NoCopy(data, dataSize))
{
}

void TStaticReader::GetStaticFile(const TString& key, TString& out) const {
    TAutoPtr<IInputStream> in;
    try {
        in = ObjectByKey(key);
    } catch (const yexception& e) {
        ythrow yexception() << "cannot get static file \"" << key << "\": archive or coding problem";
    }

    out = in->ReadAll();
}

void TStaticReader::GetStaticFile(const TString& key, IOutputStream& out) const {
    TAutoPtr<IInputStream> in;
    try {
        in = ObjectByKey(key);
    } catch (const yexception& e) {
        ythrow yexception() << "cannot get static file \"" << key << "\": archive or coding problem";
    }

    TransferData(in.Get(), &out);
}
