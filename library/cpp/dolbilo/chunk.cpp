#include "chunk.h"

#include <util/system/byteorder.h>
#include <util/generic/utility.h>
#include <util/generic/yexception.h>

class TBinaryChunkedInput::TImpl {
public:
    inline TImpl(IInputStream* input)
        : Slave_(input)
        , Pending_(Proceed())
    {
    }

    inline ~TImpl() = default;

    inline size_t Read(void* buf, size_t len) {
        if (!Pending_) {
            return 0;
        }

        const size_t toread = Min(Pending_, len);
        const size_t readed = Slave_->Read(buf, toread);

        Pending_ -= readed;

        if (!Pending_) {
            Pending_ = Proceed();
        }

        return readed;
    }

    inline size_t Proceed() {
        ui16 ret = 0;

        if (Slave_->Load(&ret, sizeof(ret)) != sizeof(ret)) {
            ythrow yexception() << "malformed binary stream";
        }

        return LittleToHost(ret);
    }

private:
    IInputStream* Slave_;
    size_t Pending_;
};

class TBinaryChunkedOutput::TImpl {
public:
    inline TImpl(IOutputStream* slave)
        : Slave_(slave)
    {
    }

    inline ~TImpl() = default;

    inline void Write(const void* buf_in, size_t len) {
        const char* buf = (const char*)buf_in;

        while (len) {
            const size_t to_write = Min<size_t>(len, 16 * 1024);

            WriteImpl(buf, to_write);

            buf += to_write;
            len -= to_write;
        }
    }

    inline void Finish() {
        WriteImpl("", 0);
    }

private:
    inline void WriteImpl(const void* buf, size_t len) {
        const ui16 llen = HostToLittle((ui16)len);

        const TPart parts[] = {
            TPart(&llen, sizeof(llen)),
            TPart(buf, len),
        };

        Slave_->Write(parts, sizeof(parts) / sizeof(*parts));
    }

private:
    IOutputStream* Slave_;
};

TBinaryChunkedInput::TBinaryChunkedInput(IInputStream* input)
    : Impl_(new TImpl(input))
{
}

TBinaryChunkedInput::~TBinaryChunkedInput() = default;

size_t TBinaryChunkedInput::DoRead(void* buf, size_t len) {
    return Impl_->Read(buf, len);
}

TBinaryChunkedOutput::TBinaryChunkedOutput(IOutputStream* output)
    : Impl_(new TImpl(output))
{
}

TBinaryChunkedOutput::~TBinaryChunkedOutput() {
    try {
        Finish();
    } catch (...) {
    }
}

void TBinaryChunkedOutput::DoWrite(const void* buf, size_t len) {
    if (Impl_.Get()) {
        Impl_->Write(buf, len);
    } else {
        ythrow yexception() << "can not write to finished stream";
    }
}

void TBinaryChunkedOutput::DoFinish() {
    if (Impl_.Get()) {
        Impl_->Finish();
        Impl_.Destroy();
    }
}
