#pragma once

#include <util/generic/ptr.h>

namespace NSnippets {
    class TArchiveView;
    class TArchiveMarkup;
    class IArchiveViewer;

    class TTrashViewer {
    public:
        TTrashViewer(const TArchiveMarkup& markup);
        ~TTrashViewer();

    public:
        const TArchiveView& GetResult() const;
        IArchiveViewer& GetViewer();

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };
}

