#include "fs_impl.h"

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////
// read & write files

void TFileSystem::Create(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name,
    mode_t mode,
    fuse_file_info* fi)
{
    const auto [flags, unsupported] = SystemFlagsToHandle(fi->flags);
    STORAGE_DEBUG("Create #" << parent
        << " " << name.Quote()
        << " flags: " << HandleFlagsToString(flags)
        << " unsupported flags: " << unsupported
        << " mode: " << mode);

    auto request = StartRequest<NProto::TCreateHandleRequest>(parent);
    request->SetName(std::move(name));
    request->SetMode(mode & ~(S_IFMT));
    request->SetFlags(flags);
    if (HasFlag(flags, NProto::TCreateHandleRequest::E_CREATE)) {
        SetUserNGroup(*request, fuse_req_ctx(req));
    }

    Session->CreateHandle(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyCreate(
                    *callContext,
                    req,
                    response.GetHandle(),
                    response.GetNodeAttr()
                );
            }
        });
}

void TFileSystem::Open(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    const auto [flags, unsupported] = SystemFlagsToHandle(fi->flags);
    STORAGE_DEBUG("Open #" << ino
        << " flags: " << HandleFlagsToString(flags)
        << " unsupported flags: " << unsupported);

    auto request = StartRequest<NProto::TCreateHandleRequest>(ino);
    request->SetFlags(flags);

    Session->CreateHandle(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                const auto& response = future.GetValue();

                fuse_file_info fi = {};
                fi.fh = response.GetHandle();

                ReplyOpen(
                    *callContext,
                    req,
                    &fi);
            }
        });
}

void TFileSystem::Release(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("Release #" << ino << " @" << fi->fh);

    auto request = StartRequest<NProto::TDestroyHandleRequest>(ino);
    request->SetHandle(fi->fh);

    Session->DestroyHandle(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::Read(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size,
    off_t offset,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("Read #" << ino << " @" << fi->fh
        << " offset:" << offset
        << " size:" << size);

    auto request = StartRequest<NProto::TReadDataRequest>(ino);
    request->SetHandle(fi->fh);
    request->SetOffset(offset);
    request->SetLength(size);

    Session->ReadData(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                const auto& buffer = response.GetBuffer();
                ReplyBuf(
                    *callContext,
                    req,
                    buffer.data(),
                    buffer.size());
            }
        });
}

void TFileSystem::Write(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    TStringBuf buffer,
    off_t offset,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("Write #" << ino << " @" << fi->fh
        << " offset:" << offset
        << " size:" << buffer.size());

    auto request = StartRequest<NProto::TWriteDataRequest>(ino);
    request->SetHandle(fi->fh);
    request->SetOffset(offset);
    request->SetBuffer(buffer.data(), buffer.size());

    Session->WriteData(callContext, std::move(request))
        .Subscribe([=, size = buffer.size()] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyWrite(
                    *callContext,
                    req,
                    size);
            }
        });
}

void TFileSystem::WriteBuf(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_bufvec* bufv,
    off_t offset,
    fuse_file_info* fi)
{
    size_t size = fuse_buf_size(bufv);
    STORAGE_DEBUG("WriteBuf #" << ino << " @" << fi->fh
        << " offset:" << offset
        << " size:" << size);

    auto buffer = TString::Uninitialized(size);

    fuse_bufvec dst = FUSE_BUFVEC_INIT(size);
    dst.buf[0].mem = (void*)buffer.data();

    ssize_t res = fuse_buf_copy(
        &dst, bufv
#if !defined(FUSE_VIRTIO)
        ,fuse_buf_copy_flags(0)
#endif
    );
    if (res < 0) {
        ReplyError(
            *callContext,
            req,
            res);
        return;
    }
    Y_VERIFY((size_t)res == size);

    auto request = StartRequest<NProto::TWriteDataRequest>(ino);
    request->SetHandle(fi->fh);
    request->SetOffset(offset);
    request->SetBuffer(std::move(buffer));

    Session->WriteData(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyWrite(
                    *callContext,
                    req,
                    size);
            }
        });
}

void TFileSystem::FAllocate(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    int mode,
    off_t offset,
    off_t length,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("FAllocate #" << ino << " @" << fi->fh
        << " offset:" << offset
        << " size:" << length);

    if (mode) {
        ReplyError(
            *callContext,
            req,
            EOPNOTSUPP);
        return;
    }

    auto request = StartRequest<NProto::TAllocateDataRequest>(ino);
    request->SetHandle(fi->fh);
    request->SetOffset(offset);
    request->SetLength(length);

    Session->AllocateData(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::Flush(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("Flush #" << ino << " @" << fi->fh);

    // TODO

    ReplyError(
        *callContext,
        req,
        0);
}

void TFileSystem::FSync(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    int datasync,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("FSync #" << ino << " @" << (fi ? fi->fh : -1llu));

    // TODO
    Y_UNUSED(datasync);

    ReplyError(
        *callContext,
        req,
        0);
}

void TFileSystem::FSyncDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    int datasync,
    fuse_file_info* fi)
{
    STORAGE_DEBUG("FSyncDir #" << ino << " @" << fi->fh);

    // TODO
    Y_UNUSED(datasync);

    ReplyError(
        *callContext,
        req,
        0);
}

}   // namespace NCloud::NFileStore::NFuse
