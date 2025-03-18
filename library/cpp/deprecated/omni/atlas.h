#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>

#include <library/cpp/json/json_reader.h>

#include "common.h"

namespace NOmni {
    /**
 * Describes Omni index "atlas" interface, a key index encoder.
 *
 * Atlas is a mapping between integers and data:
 *
 * 1 -> blob_at_0x12, 3 -> blob_at_0x192, .... n -> blob_at_0x10320F
 *
 * Depending on stored keys and data there could be different strategies for
 * optimal atlas storage. E.g. keys may follow in sequential order with or
 * without gaps, or all data blobs may have fixed size. These strategies are
 * implemented in atlas classes derived from `IAtlas`.
 *
 * Every atlas has a name which could be specified in omni index json scheme.
 * See examples.
 */
    class IAtlas {
    public:
        virtual ~IAtlas() {
        }

        virtual TBlob Archive(TFilePointersIter& iter) const {
            if (iter.AtEnd())
                return TBlob();
            Y_ENSURE(CheckKeys(iter), "bad keys for atlas");
            Y_ENSURE(CheckFileLayout(iter), "bad file pointers layout for atlas");
            return DoArchive(iter);
        }

        TFilePointer GetKeyFilePtr(const void* data, size_t dataLen, size_t key) const {
            if (!dataLen)
                return TFilePointer(-1, -1, -1);
            return DoGetKeyFilePtr((const ui8*)data, dataLen, key);
        }

        TFilePointer GetPosFilePtr(const void* data, size_t dataLen, size_t pos) const {
            Y_VERIFY(dataLen);
            return DoGetPosFilePtr((const ui8*)data, dataLen, pos);
        }

        /**
     * @returns                         Total number of keys in the given node.
     */
        size_t GetSize(const void* data, size_t dataLen) const {
            if (!dataLen)
                return 0;
            return DoGetSize((const ui8*)data, dataLen);
        }

    private:
        virtual bool CheckKeys(TFilePointersIter& iter) const = 0;
        virtual bool CheckFileLayout(TFilePointersIter& iter) const = 0;
        virtual TBlob DoArchive(TFilePointersIter& iter) const = 0;
        virtual TFilePointer DoGetKeyFilePtr(const ui8* data, ui64 dataLen, size_t key) const = 0;
        virtual TFilePointer DoGetPosFilePtr(const ui8* data, ui64 dataLen, size_t pos) const = 0;
        virtual size_t DoGetSize(const ui8* data, ui64 dataLen) const = 0;
    };

    IAtlas* NewAtlas(EAtlasType atlasType);

}
