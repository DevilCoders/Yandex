#pragma once

#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/generic/buffer.h>

namespace NDoom {


/**
 * An output stream that by default writes into internal buffer, which can
 * then be conveniently flushed into another output stream.
 */
class TAccumulatingOutput : public IOutputStream {
public:
    TAccumulatingOutput() {
        Reset();
    }

    void Reset() {
        Reservoir_.Buffer().Clear();
    }

    void Flush(IOutputStream* target) {
        TBuffer& buffer = Reservoir_.Buffer();
        target->Write(buffer.data(), buffer.size());
        buffer.Clear();
    }

    const TBuffer& Buffer() const {
        return Reservoir_.Buffer();
    }

    size_t Size() const {
        return Reservoir_.Buffer().Size();
    }

protected:
    void DoWrite(const void* buf, size_t len) override {
        Reservoir_.Write(buf, len);
    }

    void DoWriteV(const TPart* parts, size_t count) override {
        Reservoir_.Write(parts, count);
    }

private:
    TBufferOutput Reservoir_;
};


} // namespace NDoom
