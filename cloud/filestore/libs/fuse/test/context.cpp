#include "context.h"

#include <contrib/libs/virtiofsd/fuse.h>

#include <util/memory/tempbuf.h>
#include <util/system/event.h>

namespace NCloud::NFileStore::NFuse {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TBinaryReader
{
private:
    TStringBuf Buffer;

public:
    TBinaryReader(TStringBuf buffer)
        : Buffer(buffer)
    {}

    size_t Avail() const
    {
        return Buffer.size();
    }

    const char* Read(size_t count)
    {
        return Consume(count);
    }

    template <typename T>
    T Read()
    {
        const char* data = Consume(sizeof(T));

        T value;
        memcpy(&value, data, sizeof(T));

        return value;
    }

private:
    const char* Consume(size_t count)
    {
        Y_ENSURE(Buffer.size() >= count, "Invalid encoding");
        const char* p = Buffer.data();
        Buffer.Skip(count);
        return p;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TFuseContext::Playback(TStringBuf recordedRequests)
{
    static constexpr size_t MaxBufferLen = 64*1024;

    TTempBuf buffer(MaxBufferLen);
    TAutoEvent event;

    TBinaryReader reader(recordedRequests);
    while (reader.Avail()) {
        size_t requestLen = reader.Read<ui16>();
        Y_ENSURE(requestLen <= MaxBufferLen);
        Y_ENSURE(requestLen > sizeof(fuse_in_header));

        auto* in = reinterpret_cast<const fuse_in_header*>(reader.Read(requestLen));

        size_t responseLen = reader.Read<ui16>();
        Y_ENSURE(responseLen <= MaxBufferLen);

        auto* out = reinterpret_cast<fuse_out_header*>(buffer.Data());
        out->len = responseLen;

        virtio_session_enqueue(
            nullptr,
            in,
            in->len,
            out,
            out->len,
            [] (void* context, int) {
                static_cast<TAutoEvent*>(context)->Signal();
            },
            &event);

        event.Wait();

        Y_VERIFY(out->unique == in->unique);
    }
}

}   // namespace NCloud::NFileStore::NFuse
