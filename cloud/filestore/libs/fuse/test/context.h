#pragma once

#include "fuse_virtio.h"

#include <util/generic/strbuf.h>

#include <memory>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

class TFuseContext
{
private:
    ui64 RequestId = 0;

public:
    TFuseContext()
    {
        virtio_server_init();
    }

    ~TFuseContext()
    {
        virtio_server_term();
    }

    template <typename T>
    auto SendRequest(std::unique_ptr<T> request)
    {
        request->In->Header.unique = ++RequestId;

        auto future = request->Result.GetFuture();
        virtio_session_enqueue(
            nullptr,
            &request->In->Header,
            request->In->Header.len,
            &request->Out->Header,
            request->Out->Header.len,
            OnCompletion<T>,
            request.release());

        return future;
    }

    template <typename T, typename ...TArgs>
    auto SendRequest(TArgs&& ...args)
    {
        return SendRequest(std::make_unique<T>(std::forward<TArgs>(args)...));
    }

    void Playback(TStringBuf recordedRequests);

private:
    template <typename T>
    static void OnCompletion(void* context, int result)
    {
        std::unique_ptr<T> request(static_cast<T*>(context));
        request->OnCompletion(result);
    }
};

}   // namespace NCloud::NFileStore::NFuse
