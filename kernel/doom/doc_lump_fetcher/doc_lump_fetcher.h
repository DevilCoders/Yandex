#pragma once

#include <kernel/doom/wad/mapper.h>

#include <util/generic/array_ref.h>
#include <util/memory/blob.h>
#include <util/system/tls.h>

namespace NDoom {

struct TUnanswersStats {
    ui32 NumRequests = 0;
    ui32 NumUnanswers = 0;

    void MergeFrom(const TUnanswersStats& stats) {
        NumRequests += stats.NumRequests;
        NumUnanswers += stats.NumUnanswers;
    }
};

template<typename BaseLoader = IDocLumpLoader>
class IDocLumpFetcher {
public:
    using Loader = BaseLoader;

public:
    virtual TUnanswersStats Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, BaseLoader*)> cb) const = 0;

    virtual const IDocLumpMapper& Mapper() const = 0;

    virtual ~IDocLumpFetcher() {}
};

} // namespace NDoom
