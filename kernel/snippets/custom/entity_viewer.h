#pragma once

#include <util/generic/ptr.h>

namespace NSnippets {
    class TArchiveView;
    class IArchiveViewer;
    class TMetadataViewer;

    class TEntityViewer {
    public:
        TEntityViewer(const TMetadataViewer& metadataViewer);
        ~TEntityViewer();

    public:
        IArchiveViewer& GetViewer();
        const TArchiveView& GetResult() const;

    private:
        struct TImpl;
        THolder<TImpl> Impl;
    };
}
