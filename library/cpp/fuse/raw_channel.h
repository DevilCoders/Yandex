#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <library/cpp/logger/log.h>

namespace NFuse {

    struct TRawChannelOptions {
        TString Subtype = "arc";

        bool NoAutoUnmount = false;
        bool AllowRoot = false;
        bool AllowOther = false;
        bool ReadOnly = false;
        bool AutoCache = false;

        TVector<TString> Extra;
    };

    class TRawChannel : public TNonCopyable {
    public:
        TRawChannel(const TString& mountPath, const TRawChannelOptions& opts, TLog log);
        ~TRawChannel();

        int GetFd() const {
            return Fd_;
        }

        TString GetMountPath() const {
            return MountPath_;
        }

        void Unmount();

    private:
        const TString MountPath_;
        int Fd_;

        TLog Log_;
    };

} // namespace NFuse
