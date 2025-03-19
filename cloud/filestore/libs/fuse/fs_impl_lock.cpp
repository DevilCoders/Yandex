#include "fs_impl.h"

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////
// locking

void TFileSystem::GetLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi,
    struct flock* lock)
{
    if (lock->l_whence != SEEK_SET) {
        ReplyError(
            *callContext,
            req,
            EINVAL);
        return;
    }

    auto type = lock->l_type == F_RDLCK ? NProto::E_SHARED : NProto::E_EXCLUSIVE;
    TRangeLock range { fi->fh, fi->lock_owner, lock->l_start, lock->l_len };
    TestLock(callContext, req, ino, range, type);
}

void TFileSystem::SetLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi,
    struct flock* lock,
    bool sleep)
{
    if (lock->l_whence != SEEK_SET) {
        ReplyError(
            *callContext,
            req,
            EINVAL);
        return;
    }

    switch (lock->l_type) {
        case F_RDLCK: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                lock->l_start,
                lock->l_len
            };
            AcquireLock(std::move(callContext), req, ino, range, NProto::E_SHARED, sleep);
            break;
        }

        case F_WRLCK: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                lock->l_start,
                lock->l_len
            };
            AcquireLock(std::move(callContext), req, ino, range, NProto::E_EXCLUSIVE, sleep);
            break;
        }

        case F_UNLCK: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                lock->l_start,
                lock->l_len
            };
            ReleaseLock(std::move(callContext), req, ino, range);
            break;
        }

        default:
            ReplyError(
                *callContext,
                req,
                EINVAL);
            break;
    }
}

void TFileSystem::FLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi,
    int op)
{
    int realOp = op & (LOCK_EX | LOCK_SH | LOCK_UN);
    switch (realOp) {
        case LOCK_EX: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                0,
                0
            };
            bool sleep = (op & LOCK_NB) == 0;
            AcquireLock(std::move(callContext), req, ino, range, NProto::E_EXCLUSIVE, sleep);
            break;
        }

        case LOCK_SH: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                0,
                0
            };
            bool sleep = (op & LOCK_NB) == 0;
            AcquireLock(std::move(callContext), req, ino, range, NProto::E_SHARED, sleep);
            break;
        }

        case LOCK_UN: {
            TRangeLock range {
                fi->fh,
                fi->lock_owner,
                0,
                0
            };
            ReleaseLock(std::move(callContext), req, ino, range);
            break;
        }

        default:
            ReplyError(
                *callContext,
                req,
                EINVAL);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void TFileSystem::AcquireLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    const TRangeLock& range,
    NProto::ELockType type,
    bool sleep)
{
    STORAGE_TRACE("AcquireLock #" << ino);

    auto request = StartRequest<NProto::TAcquireLockRequest>(ino);
    request->SetHandle(range.Handle);
    request->SetOwner(range.Owner);
    request->SetOffset(range.Offset);
    request->SetLength(range.Length);
    request->SetLockType(type);

    Session->AcquireLock(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();

            const auto& error = response.GetError();
            if (SUCCEEDED(error.GetCode())) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            } else if (error.GetCode() == E_FS_WOULDBLOCK && sleep) {
                RequestStats->RequestCompleted(Log,*callContext);
                // locks should be acquired synchronously if asked so retry attempt
                Scheduler->Schedule(
                    Timer->Now() + Config->GetLockRetryTimeout(),
                    [=] () {
                        RequestStats->RequestStarted(Log, *callContext);
                        AcquireLock(callContext, req, ino, range, type, sleep);
                    });
            } else {
                ReplyError(
                    *callContext,
                    req,
                    ErrnoFromError(error.GetCode()));
            }
        });
}

void TFileSystem::ReleaseLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    const TRangeLock& range)
{
    STORAGE_TRACE("ReleaseLock #" << ino);

    auto request = StartRequest<NProto::TReleaseLockRequest>(ino);
    request->SetHandle(range.Handle);
    request->SetOwner(range.Owner);
    request->SetOffset(range.Offset);
    request->SetLength(range.Length);

    Session->ReleaseLock(callContext, std::move(request))
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

void TFileSystem::TestLock(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    const TRangeLock& range,
    NProto::ELockType type)
{
    STORAGE_TRACE("TestLock #" << ino);

    auto request = StartRequest<NProto::TTestLockRequest>(ino);
    request->SetHandle(range.Handle);
    request->SetOwner(range.Owner);
    request->SetOffset(range.Offset);
    request->SetLength(range.Length);
    request->SetLockType(type);

    Session->TestLock(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            const auto& error = response.GetError();
            if (!HasError(error)) {
                struct flock lock = {};
                lock.l_type = F_UNLCK;
                ReplyLock(
                    *callContext,
                    req,
                    &lock);
            } else if (error.GetCode() == E_FS_WOULDBLOCK) {
                // FIXME: NBS-2809
                // 1. return proper lock type
                // 2. return proper l_pid
                struct flock lock = {};
                lock.l_type = F_WRLCK;
                lock.l_pid = response.GetOwner();
                lock.l_whence = SEEK_SET;
                lock.l_start = response.GetOffset();
                lock.l_len = response.GetLength();

                ReplyLock(
                    *callContext,
                    req,
                    &lock);
            } else {
                ReplyError(
                    *callContext,
                    req,
                    ErrnoFromError(error.GetCode()));
            }
        });
}

}   // namespace NCloud::NFileStore::NFuse
