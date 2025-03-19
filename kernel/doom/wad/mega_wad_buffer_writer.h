#pragma once

#include <util/stream/buffer.h>

#include "mega_wad_writer.h"

namespace NDoom {

/**
 * Megawad writer that writes into internal buffer that is released with a
 * call to `Finish`.
 */
class TMegaWadBufferWriter: public TMegaWadWriter {
    using TBase = TMegaWadWriter;
public:
    TMegaWadBufferWriter(TBuffer* buffer)
        : BufferOutput_(*buffer)
    {
        Reset();
    }

    void Reset() {
        BufferOutput_.Buffer().Clear();
        TBase::Reset(&BufferOutput_);
    }

private:
    TBufferOutput BufferOutput_;
};


} // namespace NDoom
