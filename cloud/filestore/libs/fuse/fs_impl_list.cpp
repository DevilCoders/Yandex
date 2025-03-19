#include "fs_impl.h"

#include <util/generic/buffer.h>
#include <util/generic/map.h>
#include <util/random/random.h>
#include <util/system/mutex.h>

#include <sys/stat.h>

namespace NCloud::NFileStore::NFuse {

namespace {

////////////////////////////////////////////////////////////////////////////////

using TBufferPtr = std::shared_ptr<TBuffer>;

////////////////////////////////////////////////////////////////////////////////

struct TDirectoryContent {
    TBufferPtr Content = nullptr;
    size_t Offset = 0;
    size_t Size = 0;

    const char* GetData() const {
        return Content ? Content->Data() + Offset : nullptr;
    }

    size_t GetSize() const {
        return Content ? Min(Size, Content->Size() - Offset) : 0;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TDirectoryHandle
{
private:
    TMap<ui64, TBufferPtr> Content;
    TMutex Lock;

public:
    const fuse_ino_t Index;
    TString Cookie;

    TDirectoryHandle(fuse_ino_t ino)
        : Index(ino)
    {}

    TDirectoryContent UpdateContent(
        size_t size,
        size_t offset,
        const TBufferPtr& content,
        TString cookie)
    {
        with_lock (Lock) {
            size_t end = offset + content->size();
            Y_VERIFY(Content.upper_bound(end) == Content.end());
            Content[end] = content;
            Cookie = std::move(cookie);
        }

        return TDirectoryContent{content, 0, size};
    }

    TMaybe<TDirectoryContent> ReadContent(size_t size, size_t offset)
    {
        size_t end = 0;
        TBufferPtr content = nullptr;

        with_lock (Lock) {
            auto it = Content.upper_bound(offset);
            if (it != Content.end()) {
                end = it->first;
                content = it->second;
            } else if (Cookie) {
                return Nothing();
            }
        }

        TDirectoryContent result;
        if (content) {
            offset = offset - (end - content->size());
            Y_VERIFY(offset < content->size(), "off %lu size %lu",
                offset, content->size());
            result = {content, offset, size};
        }

        return result;
    }

    void ResetContent()
    {
        with_lock (Lock) {
            Content.clear();
            Cookie.clear();
        }
    }

    TString GetCookie()
    {
        with_lock (Lock) {
            return Cookie;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TDirectoryBuilder
{
private:
    TBufferPtr Buffer;

public:
    TDirectoryBuilder(size_t size) noexcept
        : Buffer(std::make_shared<TBuffer>(size))
    {}

#if defined(FUSE_VIRTIO)
    void Add(fuse_req_t req, const TString& name, const fuse_entry_param& entry, size_t offset)
    {
        size_t entrySize = fuse_add_direntry_plus(
            req,
            nullptr,
            0,
            name.c_str(),
            &entry,
            0);

        Buffer->Advance(entrySize);

        fuse_add_direntry_plus(
            req,
            Buffer->Pos() - entrySize,
            entrySize,
            name.c_str(),
            &entry,
            offset + Buffer->Size());
    }
#else
    void Add(fuse_req_t req, const TString& name, const fuse_entry_param& entry, size_t offset)
    {
        size_t entrySize = fuse_add_direntry(
            req,
            nullptr,
            0,
            name.c_str(),
            &entry.attr,
            0);

        Buffer->Advance(entrySize);

        fuse_add_direntry(
            req,
            Buffer->Pos() - entrySize,
            entrySize,
            name.c_str(),
            &entry.attr,
            offset + Buffer->Size());
    }
#endif

    TBufferPtr Finish()
    {
        return std::move(Buffer);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TFileSystem::ClearDirectoryCache()
{
    with_lock (CacheLock) {
        STORAGE_DEBUG("clear directory cache of size %lu",
            DirectoryHandles.size());
        for (auto&& [_, handler]: DirectoryHandles) {
            delete reinterpret_cast<TDirectoryHandle*>(handler);
        }
        DirectoryHandles.clear();
    }
}

////////////////////////////////////////////////////////////////////////////////
// directory listing

void TFileSystem::OpenDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("OpenDir #" << ino << " @" << fi->flags);

    ui64 id = 0;
    auto handle = std::make_unique<TDirectoryHandle>(ino);
    auto ptr = reinterpret_cast<uintptr_t>(handle.get());
    with_lock (CacheLock) {
        do {
            id = RandomNumber<ui64>();
        } while (!DirectoryHandles.try_emplace(id, ptr).second);
    }

    fuse_file_info info = {};
    info.flags = fi->flags;
    info.fh = id;

    int res = ReplyOpen(
        *callContext,
        req,
        &info);

    if (res != 0) {
        // syscall was interrupted
        handle.reset();
        with_lock (CacheLock) {
            DirectoryHandles.erase(id);
        }
    } else {
        Y_UNUSED(handle.release());
    }
}

void TFileSystem::ReadDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size,
    off_t offset,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("ReadDir #" << ino
        << " offset:" << offset
        << " size:" << size);

    TDirectoryHandle* handle = nullptr;
    with_lock (CacheLock) {
        auto it = DirectoryHandles.find(fi->fh);
        if (it == DirectoryHandles.end()) {
            ReplyError(
                *callContext,
                req,
                EBADF);
            return;
        }

        handle = reinterpret_cast<TDirectoryHandle*>(it->second);
    }

    Y_VERIFY(handle);
    Y_VERIFY(handle->Index == ino);

    auto reply = [=] (const TDirectoryContent& content) {
        ReplyBuf(
            *callContext,
            req,
            content.GetData(),
            content.GetSize());
    };

    if (!offset) {
        // directory contents need to be refreshed on rewinddir()
        handle->ResetContent();
    } else if (auto content = handle->ReadContent(size, offset)) {
        reply(*content);
        return;
    }

    auto request = StartRequest<NProto::TListNodesRequest>(ino);
    request->SetCookie(handle->GetCookie());
    request->SetMaxBytes(Config->GetMaxListBufferSize());

    Session->ListNodes(callContext, std::move(request))
        .Subscribe([=] (const auto& future) -> void {
            const auto& response = future.GetValue();
            if (!CheckResponse(*callContext, req, response)) {
                return;
            } else if (response.NodesSize() != response.NamesSize()) {
                STORAGE_ERROR("listnodes #" << fuse_req_unique(req)
                    << " names/nodes count mismatch");

                ReplyError(
                    *callContext,
                    req,
                    E_IO);
                return;
            }

            TDirectoryBuilder builder(size);
            if (offset == 0) {
                builder.Add(req, ".", { .attr = {.st_ino = MissingNodeId}}, offset);
                builder.Add(req, "..", { .attr = {.st_ino = MissingNodeId}}, offset);
            }

            for (size_t i = 0; i < response.NodesSize(); ++i) {
                const auto& attr = response.GetNodes(i);
                const auto& name = response.GetNames(i);

                fuse_entry_param entry = {
                    .ino = attr.GetId(),
                    .attr_timeout = (double)Config->GetAttrTimeout().Seconds(),
                    .entry_timeout = (double)Config->GetEntryTimeout().Seconds(),
                };

                ConvertAttr(Config->GetBlockSize(), attr, entry.attr);
                if (!entry.attr.st_ino) {
                    STORAGE_ERROR("#" << fuse_req_unique(req)
                        << " listed invalid entry: name " << name.Quote()
                        << ", stat " << DumpMessage(attr));
                    ReplyError(
                        *callContext,
                        req,
                        E_IO);
                    return;
                }

                builder.Add(req, name, entry, offset);
            }

            auto content = handle->UpdateContent(
                size,
                offset,
                builder.Finish(),
                response.GetCookie());

            STORAGE_TRACE("# " << fuse_req_unique(req)
                << " offset: " << offset
                << " limit: " << size
                << " actual size " << content.GetSize());

            reply(content);
        });
}

void TFileSystem::ReleaseDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("ReleaseDir #" << ino);

    TDirectoryHandle* handle = nullptr;
    with_lock (CacheLock) {
        auto it = DirectoryHandles.find(fi->fh);
        if (it != DirectoryHandles.end()) {
            handle = reinterpret_cast<TDirectoryHandle*>(it->second);
            DirectoryHandles.erase(it);
        }
    }

    if (handle) {
        Y_VERIFY(handle->Index == ino);
        delete handle;
    }

    // should reply w/o lock
    ReplyError(
        *callContext,
        req,
        0);
}

}   // namespace NCloud::NFileStore::NFuse
