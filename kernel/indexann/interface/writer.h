#pragma once

#include <util/generic/array_ref.h>


namespace NIndexAnn {

    class IDocDataWriter {
    public:
        virtual void StartDoc(ui32 docId) = 0;
        virtual void FinishDoc() = 0;
        virtual void Add(ui32 breakId, ui32 regionId, ui32 streamId, TArrayRef<const char> data) = 0;
        virtual ~IDocDataWriter() {
        }
    };

} // namespace NIndexAnn

