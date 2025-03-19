#pragma once

#include <cloud/filestore/libs/service/filestore.h>

#include <library/cpp/threading/future/future.h>

#include <contrib/libs/virtiofsd/fuse.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

template <typename THeader, typename TBody>
struct TRequestBuffer
{
    THeader Header;
    TBody Body;

    TRequestBuffer(size_t len)
    {
        Zero(*this);
        Header.len = len;
    }

    void* Data() const
    {
        return (void*)(this + 1);    // variable-sized structure
    }

    static auto Create(size_t dataSize = 0)
    {
        size_t len = sizeof(TRequestBuffer<THeader, TBody>) + dataSize;
        void* buffer = ::operator new(len);
        return std::unique_ptr<TRequestBuffer<THeader, TBody>> {
            new (buffer) TRequestBuffer<THeader, TBody>(len)
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

template<typename TInPayload, typename TOutPayload, typename TResult>
struct TRequestBase
{
    using TIn = TRequestBuffer<fuse_in_header, TInPayload>;
    using TOut = TRequestBuffer<fuse_out_header, TOutPayload>;

    std::unique_ptr<TIn> In = TIn::Create();
    std::unique_ptr<TOut> Out = TOut::Create();

    NThreading::TPromise<TResult> Result = NThreading::NewPromise<TResult>();

    virtual ~TRequestBase() = default;

    void OnCompletion(int result)
    {
        Y_UNUSED(result);

        if (Out->Header.error) {
            Result.SetException(LastSystemErrorText(Out->Header.error));
            return;
        }

        SetResult();
    }

    virtual void SetResult() = 0;
};


template<typename TInPayload, typename TOutPayload>
struct TRequestBase<TInPayload, TOutPayload, void>
{
    using TIn = TRequestBuffer<fuse_in_header, TInPayload>;
    using TOut = TRequestBuffer<fuse_out_header, TOutPayload>;

    std::unique_ptr<TIn> In = TIn::Create();
    std::unique_ptr<TOut> Out = TOut::Create();

    NThreading::TPromise<void> Result = NThreading::NewPromise();

    void OnCompletion(int result)
    {
        Y_UNUSED(result);

        if (Out->Header.error) {
            Result.SetException(LastSystemErrorText(Out->Header.error));
            return;
        }

        Result.SetValue();
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TInitRequest: public TRequestBase<fuse_init_in, fuse_init_out, void>
{
    static constexpr int DefaultFlags =
        FUSE_ASYNC_READ | FUSE_POSIX_LOCKS | FUSE_ATOMIC_O_TRUNC |
        FUSE_EXPORT_SUPPORT | FUSE_BIG_WRITES | FUSE_DONT_MASK |
        FUSE_SPLICE_WRITE | FUSE_SPLICE_MOVE | FUSE_SPLICE_READ |
        FUSE_FLOCK_LOCKS | FUSE_HAS_IOCTL_DIR | FUSE_AUTO_INVAL_DATA |
        FUSE_DO_READDIRPLUS | FUSE_READDIRPLUS_AUTO | FUSE_ASYNC_DIO |
        FUSE_WRITEBACK_CACHE | FUSE_NO_OPEN_SUPPORT |
        FUSE_PARALLEL_DIROPS | FUSE_HANDLE_KILLPRIV | FUSE_POSIX_ACL |
        FUSE_ABORT_ERROR | FUSE_MAX_PAGES | FUSE_CACHE_SYMLINKS |
        FUSE_NO_OPENDIR_SUPPORT | FUSE_EXPLICIT_INVAL_DATA;

    explicit TInitRequest(int flags = DefaultFlags)
    {
        In->Header.opcode = FUSE_INIT;
        In->Body.major = FUSE_KERNEL_VERSION;
        In->Body.minor = FUSE_KERNEL_MINOR_VERSION;
        In->Body.flags = flags;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TDestroyRequest: public TRequestBase<ui32, ui32, void>
{
    TDestroyRequest()
    {
        In->Header.opcode = FUSE_DESTROY;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TCreateOutData
{
    fuse_entry_out Entry;
    fuse_open_out Open;
};

struct TCreateHandleRequest
    : public TRequestBase<fuse_create_in, TCreateOutData, ui64>
{
    TCreateHandleRequest(const TString& name, ui64 nodeId = RootNodeId)
    {
        In = TIn::Create(name.size() + 1);
        In->Header.opcode = FUSE_CREATE;
        In->Header.nodeid = nodeId;
        strcpy((char*)In->Data(), name.c_str());
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body.Open.fh);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TLookupRequest: public TRequestBase<char[4096], fuse_entry_out, ui64>
{
    TLookupRequest(const TString& name, ui64 nodeId = RootNodeId)
    {
        Y_VERIFY(name.size() < 4096);

        In->Header.opcode = FUSE_LOOKUP;
        In->Header.nodeid = nodeId;
        strcpy(In->Body, name.c_str());
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body.nodeid);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteRequest: public TRequestBase<fuse_write_in, fuse_write_out, ui32>
{
    TWriteRequest(ui64 nodeId, ui64 handle, ui64 offset, TStringBuf buffer)
    {
        In = TIn::Create(buffer.size());
        In->Header.opcode = FUSE_WRITE;
        In->Header.nodeid = nodeId;
        In->Body.fh = handle;
        In->Body.offset = offset;
        In->Body.size = buffer.size();
        memcpy(In->Data(), buffer.data(), buffer.size());
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body.size);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TOpenDirRequest: public TRequestBase<fuse_open_in, fuse_open_out, ui64>
{
    TOpenDirRequest(ui64 nodeId)
    {
        In->Header.opcode = FUSE_OPENDIR;
        In->Header.nodeid = nodeId;
        In->Body.flags = 0;
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body.fh);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TReadDirRequest: public TRequestBase<fuse_read_in, int, ui32>
{
    TReadDirRequest(ui64 nodeId, ui64 fh, ui64 offset = 0, ui32 size = 4096)
    {
        In->Header.opcode = FUSE_READDIRPLUS;
        In->Header.nodeid = nodeId;
        In->Body.fh = fh;
        In->Body.offset = offset;
        In->Body.size = size;

        Out = TOut::Create(size);
    }

    void SetResult() override
    {
        Result.SetValue(Out->Header.len - sizeof(Out->Header));
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TReleaseDirRequest: public TRequestBase<fuse_release_in, int, void>
{
    TReleaseDirRequest(ui64 nodeId, ui64 fh)
    {
        In->Header.opcode = FUSE_RELEASEDIR;
        In->Header.nodeid = nodeId;
        In->Body.fh = fh;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TForgetRequest: public TRequestBase<fuse_forget_in, int, void>
{
    TForgetRequest(ui64 nodeId, ui64 c)
    {
        In->Header.opcode = FUSE_FORGET;
        In->Header.nodeid = nodeId;
        In->Body.nlookup = c;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TAcquireLockRequest: public TRequestBase<fuse_lk_in, int, void>
{
    TAcquireLockRequest(ui64 flags, ui64 type, bool setlkw = true)
    {
        In->Header.opcode = setlkw ? FUSE_SETLKW : FUSE_SETLK;
        // possible flags FUSE_LK_FLOCK
        In->Body.lk_flags = flags;
        In->Body.lk.type = type;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTestLockRequest
    : public TRequestBase<fuse_lk_in, fuse_lk_out, fuse_file_lock>
{
    TTestLockRequest(ui64 flags, ui64 type, ui64 start, ui64 end)
    {
        In->Header.opcode = FUSE_GETLK;
        In->Body.lk_flags = flags;
        In->Body.lk.type = type;
        In->Body.lk.start = start;
        In->Body.lk.type = end;
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body.lk);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TGetXAttrValueRequest
    : public TRequestBase<fuse_getxattr_in, char[4096], TString>
{
    TGetXAttrValueRequest(const TString& attr, size_t size, ui64 nodeId = RootNodeId)
    {
        Y_VERIFY(size < 4096);

        In = TIn::Create(attr.size() + 1);
        In->Header.opcode = FUSE_GETXATTR;
        In->Header.nodeid = nodeId;
        In->Body.size = size;
        strcpy((char*)In->Data(), attr.c_str());
    }

    void SetResult() override
    {
        Result.SetValue(Out->Body);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TSetXAttrValueRequest
    : public TRequestBase<fuse_setxattr_in, int, ui32>
{
    TSetXAttrValueRequest(const TString& name, const TString& value, ui64 nodeId = RootNodeId)
    {
        In = TIn::Create(name.size() + value.size() + 2);
        In->Header.opcode = FUSE_SETXATTR;
        In->Header.nodeid = nodeId;
        In->Body.size = value.size();
        strcpy((char*)In->Data(), name.c_str());
        strcpy((char*)In->Data() + name.size() + 1, value.c_str());
    }

    void SetResult() override
    {
        Result.SetValue(Out->Header.error);
    }
};

}   // namespace NCloud::NFileStore::NFuse
