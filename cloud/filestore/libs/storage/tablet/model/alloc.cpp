#include "alloc.h"

#include <util/generic/singleton.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

TProfilingAllocator* GetAllocatorByTag(EAllocatorTag tag)
{
    static auto* r = Singleton<TProfilingAllocatorRegistry<EAllocatorTag>>();
    return r->GetAllocator(tag);
}

}   // namespace NCloud::NFileStore::NStorage
