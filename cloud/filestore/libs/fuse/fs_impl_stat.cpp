#include "fs_impl.h"

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////
// filesystem information

void TFileSystem::StatFs(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino)
{
    STORAGE_DEBUG("StatFs #" << ino);

    auto request = StartRequest<NProto::TStatFileStoreRequest>();

    Session->StatFileStore(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                const auto& info = response.GetFileStore();
                const auto& stats = response.GetStats();

                struct statvfs st;
                ConvertStat(info, stats, st);

                ReplyStatFs(
                    *callContext,
                    req,
                    &st);
            }
        });
}

}   // namespace NCloud::NFileStore::NFuse
