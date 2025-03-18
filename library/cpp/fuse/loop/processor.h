#pragma once

#include <library/cpp/fuse/raw_channel.h>
#include <library/cpp/fuse/fd_channel.h>
#include <library/cpp/fuse/low_mount.h>

#include <util/generic/ptr.h>

namespace NFuse {

    class TLowMountProcessor final : public TAtomicRefCount<TLowMountProcessor> {
    public:
        TLowMountProcessor(
            const TString& mountPath,
            const ::NFuse::TRawChannelOptions& channelOptions,
            ::NFuse::TLowMount::TOptions lowMountOptions
        );
        ~TLowMountProcessor();

        void SetIOThreadCount(int count);

        EMessageStatus Receive(TBuffer& buf);
        void Process(TStringBuf buf, int threadIdx);

        bool WaitInitD(TInstant deadline);
        void Shutdown(const TString& reason);

        TString GetMountPath() const;
        int GetChannelFd() const;

        TManualEvent GetMountDeadEvent() const;

    private:
        ::NFuse::TRawChannel RawChannel_;
        TIntrusivePtr<::NFuse::TFdChannel> FdChannel_;
        TIntrusivePtr<::NFuse::TLowMount> Mount_;
        TManualEvent DeadEvent_;
    };

    using TMountProcessorRef = TIntrusivePtr<TLowMountProcessor>;

} // namespace NVcs::NFuse
