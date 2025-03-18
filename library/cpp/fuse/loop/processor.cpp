#include "processor.h"

namespace NFuse {

    TLowMountProcessor::TLowMountProcessor(
        const TString& mountPath,
        const ::NFuse::TRawChannelOptions& channelOptions,
        ::NFuse::TLowMount::TOptions lowMountOptions
    )
        : RawChannel_(mountPath, channelOptions, lowMountOptions.Log)
        , FdChannel_(MakeIntrusive<::NFuse::TFdChannel>(RawChannel_.GetFd(),
                                                        lowMountOptions.Log,
                                                        mountPath))
    {
        lowMountOptions.Channel = FdChannel_;
        Mount_ = MakeIntrusive<::NFuse::TLowMount>(lowMountOptions);
    }

    TLowMountProcessor::~TLowMountProcessor() {
        Shutdown("Processor destroyed");
        DeadEvent_.Signal();
    }

    void TLowMountProcessor::SetIOThreadCount(int count) {
        Mount_->SetIOThreadCount(count);
    }

    EMessageStatus TLowMountProcessor::Receive(TBuffer& buf) {
        return FdChannel_->Receive(buf);
    }

    void TLowMountProcessor::Process(TStringBuf buf, int threadIdx) {
        Mount_->Process(buf, threadIdx);
    }

    TString TLowMountProcessor::GetMountPath() const {
        return RawChannel_.GetMountPath();
    }

    int TLowMountProcessor::GetChannelFd() const {
        return RawChannel_.GetFd();
    }

    TManualEvent TLowMountProcessor::GetMountDeadEvent() const {
        return DeadEvent_;
    }

    bool TLowMountProcessor::WaitInitD(TInstant deadline) {
        return Mount_->WaitInitD(deadline);
    }

    void TLowMountProcessor::Shutdown(const TString& reason) {
        if (Mount_) {
            Mount_->Shutdown(reason);
            Mount_.Drop();
            FdChannel_.Drop();
            RawChannel_.Unmount();
        }
    }

} // namespace NVcs::NFuse

