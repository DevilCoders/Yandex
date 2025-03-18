#include "fatinv.h"

#include <kernel/keyinv/indexfile/fat.h>
#include <kernel/keyinv/indexfile/memoryportion.h>
#include <kernel/keyinv/indexfile/indexutil.h>

#include <util/stream/str.h>

typedef NIndexerCore::NIndexerDetail::TInputMemoryStream    TInMemStream;
typedef NIndexerCore::TInputIndexFileImpl<TInMemStream>     TInMemIndexFile;

//TODO: merge back to library/indexfile/indexutil?

void AddFat(TString& skey, TString& sinv) {

    TInMemStream keyStream(skey.data(), skey.size());
    TInMemStream invStream(sinv.data(), sinv.size());
    TInMemIndexFile im(keyStream, invStream, IYndexStorage::FINAL_FORMAT);
    NIndexerCore::TInvKeyReaderImpl<TInMemIndexFile> keyReader(im, false);

    TStringStream ss;

    ui32 keyBufLen = 0;
    ui32 numKeys = 0;
    ui32 numBlocks = 0;

    const ui32 version = keyReader.GetIndexVersion();
    NIndexerCore::WriteVersionData(ss, version);

    if (version & YNDEX_VERSION_FLAG_KEY_COMPRESSION) {
        Y_VERIFY(false);
    }

    bool first = true;
    while (true) {
        bool needNextBlock = keyReader.NeedNextBlock();
        if (!keyReader.ReadNext()) {
            break;
        }
        if (needNextBlock) {
            if (!first) {
                ss.Write(&numKeys, 4);
                keyBufLen += 4;
            }
            first = false;
            numBlocks++;
            const size_t len = strlen(keyReader.GetKeyText()) + 1;
            ss.Write(keyReader.GetKeyText(), len);
            keyBufLen += (ui32)len;
            i64 offset = keyReader.GetOffset();
            ss.Write(&offset, 8);
            keyBufLen += 8;

        }
        numKeys++;
    }

    if (!first) {
        // previous incomplete block counter
        ss.Write(&numKeys, 4);
        keyBufLen += 4;
    }

    NIndexerCore::WriteIndexStat(ss, keyReader.GetSubIndexInfo().hasSubIndex, keyBufLen, numKeys, numBlocks);

    sinv += ss.Str();
}
