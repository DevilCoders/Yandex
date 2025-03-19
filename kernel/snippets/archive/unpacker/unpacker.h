#pragma once

#include <kernel/snippets/iface/archive/enums.h>
#include <kernel/snippets/iface/archive/manip.h>

#include <util/generic/ptr.h>

namespace NSnippets
{
    class TConfig;
    class ISentsFilter;
    class TArchiveView;
    class IArchiveViewer;
    class TSentsOrder;
    class TArchiveStorage;
    class TArchiveMarkup;
    class TUnpacker : public IArchiveConsumer {
    private:
        class TImpl;
        THolder<TImpl> Impl;

    public:
        TUnpacker(const TConfig& cfg, TArchiveStorage* storage, TArchiveMarkup* markup, EARC sourceArc);
        ~TUnpacker() override;

        const TArchiveView& GetAllUnpacked() const;
        void AddRequest(TSentsOrder& order);
        void AddRequester(IArchiveViewer* viewer);

        int UnpText(const ui8* doctext) override;
    };
}
