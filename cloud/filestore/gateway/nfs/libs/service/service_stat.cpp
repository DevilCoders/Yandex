#include "service.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int TFileStoreService::StatFs(ui64 ino, struct statfs* stat)
{
    STORAGE_TRACE("StatFs #" << ino);

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TGetFileStoreInfoRequest>();

    auto future = Session->GetFileStoreInfo(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    ConvertStat(response.GetFileStore(), *stat);
    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
