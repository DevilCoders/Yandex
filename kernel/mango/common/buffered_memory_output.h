#pragma once

#include <util/stream/output.h>
#include <util/generic/noncopyable.h>
#include <util/generic/buffer.h>

class TBufferedMemoryOutput : public IOutputStream
{
    TBuffer Buf;
    size_t WarnLimit;
public:
    TBufferedMemoryOutput(size_t warnLimit = 0)
        : Buf(warnLimit), WarnLimit(warnLimit)
    {}

    ~TBufferedMemoryOutput() override
    {}

    size_t Size() const
    {
        return Buf.Size();
    }

    const char* Data() const
    {
        return Buf.Data();
    }

    bool LimitReached() const
    {
        return WarnLimit != 0 && Buf.Size() >= WarnLimit;
    }

    void Clear()
    {
        Buf.Clear();
    }

    bool Empty() const
    {
        return Buf.Empty();
    }

protected:
    void DoWrite(const void* buf, size_t len) noexcept override
    {
        Buf.Append(reinterpret_cast<const char*>(buf), len);
    }
};
