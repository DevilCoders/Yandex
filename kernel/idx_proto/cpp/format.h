#pragma once

#include <kernel/idx_proto/feature_pool.pb.h>

#include <kernel/idx_ops/types.h>
#include <kernel/idx_ops/req_less.h>

#include <util/generic/yexception.h>
#include <util/generic/buffer.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/memory/blob.h>

namespace NIdxOps {

// Shard -> TShardInfo mapping
typedef THashMap<TString, NFeaturePool::TShardInfo> TShardInfos;

// Widely used array of proto-lines: for search results, merging, etc
typedef TVector<NFeaturePool::TLine> TProtoLines;

// Reader and writer interface,
// for implementation see search/tools/idx_proto
template <class Item>
class IReader {
public:
    virtual bool Next(Item& item) = 0;
    virtual ~IReader() {}
};

template <class Item>
class IWriter {
public:
    virtual void Write(const Item& item) = 0;
    virtual void Close() {}
    virtual ~IWriter() {}
};

using ILineReader = IReader<NFeaturePool::TLine>;
using ILineWriter = IWriter<NFeaturePool::TLine>;

struct TKey {
    TRequestId RequestId;
    ui64 UrlHash;
    TKey()
        : RequestId(0)
        , UrlHash(0)
    { }

    inline bool operator<(const TKey& rhs) const {
        if (RequestId == rhs.RequestId)
            return UrlHash < rhs.UrlHash;
        return StrLess(RequestId, rhs.RequestId);
    }
    inline bool operator==(const TKey& rhs) const {
        return (RequestId == rhs.RequestId) && (UrlHash == rhs.UrlHash);
    }
};

struct TSortableLine {
    TKey Key;
    TBlob Blob;
    bool Final;

    explicit TSortableLine(bool final = false)
        : Final(final)
    { }

    TSortableLine(const char* data, size_t size)
        : Final(false)
    {
        Blob = TBlob::Copy(data, size);
    }

    inline bool operator<(const TSortableLine& rhs) const {
        return Key < rhs.Key;
    }
    inline bool operator==(const TSortableLine& rhs) const {
        return Key == rhs.Key;
    }

    inline bool Empty() const {
        return (Blob.Size() == 0);
    }

    inline void Read(IInputStream& input) {
        ui32 blobSize = 0;
        size_t readSize = input.Load(&blobSize, sizeof(blobSize));
        if (!readSize) { // EOF reached
            Blob.Drop();
            return;
        }
        Y_ENSURE(readSize == sizeof(blobSize), "Cannot read blob size. ");
        Y_ENSURE(blobSize > 0, "Zero blob size read. ");

        TBuffer buf;
        buf.Resize(blobSize);

        input.LoadOrFail(&Key, sizeof(TKey));
        input.LoadOrFail(buf.Data(), blobSize);
        Blob = TBlob::FromBuffer(buf);
    }

    inline void Write(IOutputStream& output) {
        ui32 blobSize = ui32(Blob.Size());
        Y_ENSURE(blobSize != 0, "Zero blob size written. ");
        output.Write(&blobSize, sizeof(blobSize));
        output.Write(&Key, sizeof(TKey));
        output.Write(Blob.Data(), blobSize);
    }
};

} // namespace NIdxOps
