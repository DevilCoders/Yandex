#include "http_static.h"

#include <library/cpp/archive/yarchive.h>

#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>

namespace {
    static const unsigned char data[] = {
        #include "http_static.inc"
    };

    class TStaticArchive: public TArchiveReader {
    public:
        inline TStaticArchive()
            : TArchiveReader(TBlob::NoCopy(data, sizeof(data)))
        {
        }
    };

}

void GetStaticFile(const TString& key, TString& out) {
    TAutoPtr<IInputStream> in;
    try {
        in = Singleton<TStaticArchive>()->ObjectByKey(key);
    } catch (const yexception& e) {
        ythrow yexception() << "cannot get static file \"" << key << "\": archive or coding problem";
    }

    out = in->ReadAll();
}

void GetStaticFile(const TString& key, IOutputStream& out) {
    TAutoPtr<IInputStream> in;
    try {
        in = Singleton<TStaticArchive>()->ObjectByKey(key);
    } catch (const yexception& e) {
        ythrow yexception() << "cannot get static file \"" << key << "\": archive or coding problem";
    }

    TransferData(in.Get(), &out);
}
