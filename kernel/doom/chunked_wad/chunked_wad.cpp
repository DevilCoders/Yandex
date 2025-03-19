#include "chunked_wad.h"

#include "chunked_mega_wad.h"
#include "single_chunked_wad.h"

namespace NDoom {

THolder<IChunkedWad> IChunkedWad::OpenChunked(
    const TString& prefix,
    bool lockMemory)
{
    return MakeHolder<TChunkedMegaWad>(prefix, lockMemory);
}

THolder<IChunkedWad> IChunkedWad::Open(const TString& indexPath, bool lockMemory) {
    return MakeHolder<TSingleChunkedWad>(IWad::Open(indexPath, lockMemory));
}

THolder<IChunkedWad> IChunkedWad::Open(const TArrayRef<const char>& source) {
    return MakeHolder<TSingleChunkedWad>(IWad::Open(source));
}

THolder<IChunkedWad> IChunkedWad::Open(TBuffer&& buffer) {
    return MakeHolder<TSingleChunkedWad>(IWad::Open(std::move(buffer)));
}

} // namespace
