#pragma once

#include "buffer.h"
#include "direct_io_file_read_request.h"

#include <contrib/libs/libaio/libaio.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>
#include <util/memory/blob.h>
#include <util/system/fhandle.h>

#include <cerrno>

namespace NDoom {

int user_io_getevents(io_context_t aio_ctx, unsigned int max, struct io_event *events);

class TAsyncDirectIoThreadLocalReader: public TNonCopyable {
public:
    enum EBlockState {
        FreeBlockState,
        PreparedBlockState,
        SubmittedBlockState,
        ReadyBlockState
    };

    template<typename T>
    class TNoopIterator {
    public:
        TNoopIterator& operator* () { return *this; }
        TNoopIterator& operator++ () { return *this; }
        TNoopIterator& operator= (T&&) { return *this; }
    };

public:
    explicit TAsyncDirectIoThreadLocalReader(size_t maxEvents = 32)
        : MaxEvents_(maxEvents)
        , FreeBlocks_(maxEvents)
    {
        InitContext();
    }

    ~TAsyncDirectIoThreadLocalReader() {
        DestroyContext();
    }

    void DestroyContext() {
        if (Inited_) {
            Y_VERIFY(io_destroy(IoContext_) == 0);
            IoContext_ = nullptr;
            Inited_ = false;
        }
    }

    size_t MaxEvents() const {
        return MaxEvents_;
    }

    bool IsReady(size_t index) const {
        Y_ENSURE(Inited_);
        return BlockStates_[index] == ReadyBlockState;
    }

    TBlob MoveResult(size_t index) {
        Y_ENSURE(Inited_);
        Y_ENSURE(BlockStates_[index] == ReadyBlockState);
        BlockStates_[index] = FreeBlockState;
        FreeBlocks_.PushBack(index);
        return Requests_[index].MoveResult();
    }

    void Lock() {
        Locked_ = true;
    }

    void Unlock() {
        Locked_ = false;
    }

    size_t AllocateBlocks(TArrayRef<ui32> reqIds) {
        Y_VERIFY(!Locked_);
        InitContext();

        for (size_t i = 0; i < reqIds.size(); ++i) {
            if (FreeBlocks_.Empty()) {
                return i;
            } else {
                size_t id = FreeBlocks_.Head();
                Y_VERIFY(BlockStates_[id] == FreeBlockState);
                FreeBlocks_.Advance(1);
                reqIds[i] = id;
            }
        }

        return reqIds.size();
    }

    void PrepareBlock(FHANDLE handle, ui64 offset, ui64 size, ui32 id, TMaybe<int> resFd = Nothing()) {
        TDirectIoFileReadRequest readRequest(offset, size);

#if defined(__has_feature)
#if  __has_feature(memory_sanitizer)
        memset(readRequest.AlignedBuffer(), 0, readRequest.AlignedSize());
#endif
#endif


        Y_ENSURE(BlockStates_[id] == FreeBlockState);
        Requests_[id] = std::move(readRequest);
        BlockStates_[id] = PreparedBlockState;

        io_prep_pread(
            &(Cbs_[id]),
            handle,
            Requests_[id].AlignedBuffer(),
            Requests_[id].AlignedSize(),
            Requests_[id].AlignedOffset());

        if (resFd) {
            io_set_eventfd(&(Cbs_[id]), *resFd);
        }

        ToSend_.push_back(&Cbs_[id]);
    }

    size_t PartialPrepare(
        const TArrayRef<const FHANDLE>& files,
        const TArrayRef<const ui64>& offsets,
        const TArrayRef<const ui64>& sizes,
        TArrayRef<ui32> reqIds,
        TMaybe<int> resFd = Nothing())
    {
        size_t result = AllocateBlocks(reqIds);

        for (size_t i = 0; i < result; ++i) {
            PrepareBlock(files[i], offsets[i], sizes[i], reqIds[i], resFd);
        }

        return result;
    }

    size_t SendQueueSize() const {
        return ToSend_.size();
    }

    size_t ProcessingRequests() const {
        return SentRequests_ + SendQueueSize();
    }

    size_t Submit() {
        if (ToSend_) {
            const int ret = io_submit(IoContext_, ToSend_.size(), &ToSend_[0]);
            SentRequests_ += ret;
            if (ret < 0) {
                DestroyContext();
                Y_ENSURE(false);
            }
            for (int i = 0; i < ret; ++i) {
                BlockStates_[ToSend_[i] - &Cbs_[0]] = SubmittedBlockState;
            }
            for (size_t i = 0; i + ret < ToSend_.size(); ++i) {
                ToSend_[i] = ToSend_[i + ret];
            }
            ToSend_.resize(ToSend_.size() - ret);
            return ret;
        } else {
            return 0;
        }
    }

    void Submit(const TArrayRef<const FHANDLE>& files, const TArrayRef<const ui64>& offsets, const TArrayRef<const ui64>& sizes) {
        Y_VERIFY(!Locked_);
        if (SentRequests_ > 0) {
            DestroyContext();
        }

        InitContext();

        Y_ENSURE(files.size() <= MaxEvents_);
        Y_ENSURE(files.size() == offsets.size());
        Y_ENSURE(files.size() == sizes.size());

        Y_ENSURE(SentRequests_ == 0);
        Y_ENSURE(SendQueueSize() == 0);

        TVector<ui32> ids(files.size());

        FreeBlocks_.Clear();
        for (size_t i = 0; i < files.size(); ++i) {
            PrepareBlock(files[i], offsets[i], sizes[i], i);
        }
        while (SendQueueSize() > 0) {
            Submit();
        }

        memset(Events_.data(), 0, sizeof(io_event) * files.size());

        SentRequests_ = files.size();
    }

    void Cancel() {
        for (size_t index = 0; index < MaxEvents_; ++index) {
            if (BlockStates_[index] == SubmittedBlockState) {
                int ret;
                do {
                    ret = io_cancel(IoContext_, &Cbs_[index], &Events_[index]);
                } while (ret == -EAGAIN);

                if (ret) {
                    DestroyContext();
                    Y_ENSURE(false);
                }
            }
        }
    }

    template<typename Out = TNoopIterator<ui32>>
    void GetAllEvents(Out out = Out()) {
        while (SentRequests_ > 0) {
            GetEvents(SentRequests_, nullptr, out);
        }
    }

    template<typename Out = TNoopIterator<ui32>>
    void Poll(Out out = Out()) {
        GetEvents(1, nullptr, out);
    }

    template<typename Out = TNoopIterator<ui32>>
    void GetReadyEvents(Out out = Out()) {
        //GetReadyEvents(TDuration::Zero(), out);
        CollectEvents(user_io_getevents(IoContext_, SentRequests_, &Events_[0]), out);
    }

    template<typename Out = TNoopIterator<ui32>>
    void GetReadyEvents(const TDuration& timeout, Out out = Out()) {
        if (SentRequests_ == 0) {
            return;
        }
        timespec spec;
        spec.tv_sec = timeout.Seconds();
        spec.tv_nsec = timeout.NanoSecondsOfSecond();
        return GetEvents(SentRequests_, &spec, out);
    }

private:
    template<typename Out = TNoopIterator<ui32>>
    void GetEvents(size_t minEvents, timespec* timeout = nullptr, Out out = Out()) {
        Y_ENSURE(Inited_);
        const int events = io_getevents(IoContext_, minEvents, SentRequests_, &Events_[0], timeout);
        CollectEvents(events, out);
    }

    template<typename Out = TNoopIterator<ui32>>
    void CollectEvents(int events, Out out = Out()) {
        if (events < 0) {
            DestroyContext();
            Y_ENSURE(false);
        }
        for (int i = 0; i < events; ++i) {
            size_t index = Events_[i].obj - &(Cbs_[0]);
            *out = index;
            ++out;
            if (index >= Cbs_.size() || BlockStates_[index] != SubmittedBlockState) {
                DestroyContext();
                Y_ENSURE(false);
            }
            const long ret2 = Events_[i].res;
            if (ret2 < 0 || Events_[i].res < Requests_[index].ExpandedSize()) {
                DestroyContext();
                Y_ENSURE(false);
            }
            BlockStates_[index] = ReadyBlockState;
        }
        SentRequests_ -= events;
    }

    void InitContext() {
        if (!Inited_) {
            Y_ENSURE(io_setup(MaxEvents_, &IoContext_) == 0);
            Inited_ = true;

            Requests_.resize(MaxEvents_);
            Cbs_.resize(MaxEvents_);
            BlockStates_.assign(MaxEvents_, FreeBlockState);

            ToSend_.reserve(MaxEvents_);
            ToSend_.resize(0);

            Events_.resize(MaxEvents_);

            FreeBlocks_.Clear();
            for (size_t i = 0; i < MaxEvents_; ++i) {
                FreeBlocks_.PushBack(i);
            }
        }
    }


    bool Inited_ = false;
    io_context_t IoContext_ = 0;
    size_t MaxEvents_ = 32;

    size_t SentRequests_ = 0;
    TVector<TDirectIoFileReadRequest> Requests_;
    TVector<iocb> Cbs_;
    TVector<iocb*> ToSend_;

    TVector<io_event> Events_;
    TVector<EBlockState> BlockStates_;
    bool Locked_ = false;

    TRingArray<size_t> FreeBlocks_;
};


} // namespace NDoom
