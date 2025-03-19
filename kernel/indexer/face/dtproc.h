#pragma once

#include <kernel/indexer/direct_text/dt.h>

#include <util/generic/ptr.h>

class IDirectTextProcessor : public TSimpleRefCount<IDirectTextProcessor>
                           , public NIndexerCore::IDirectTextCallback2
{
public:
    // textProcessorDir = DatHome + "config/" + IndexProcessorParser_Name
    virtual void SetProcessorDir(const char* /*DatHome*/, const char* /*sCluster*/) {
    }
};
