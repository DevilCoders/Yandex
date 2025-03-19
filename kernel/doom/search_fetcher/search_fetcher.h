#pragma once

#include <kernel/doom/doc_lump_fetcher/doc_lump_fetcher.h>
#include <kernel/doom/doc_lump_fetcher/consistent_fetcher.h>

#include <kernel/doom/chunked_wad/mapper.h>
#include <kernel/doom/wad/multi_mapper.h>


namespace NDoom {

using TSearchDocLoader = NDoom::TConsistentDocLumpLoader<NDoom::TMultiDocLumpLoader<NDoom::TChunkedWadDocLumpLoader>>;
using ISearchDocFetcher = NDoom::IDocLumpFetcher<TSearchDocLoader>;

} // namespace NDoom
