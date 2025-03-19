#pragma once

#include <contrib/libs/grpc/include/grpcpp/grpcpp.h>

#include <library/cpp/logger/log.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <functional>

namespace NKMS {
    struct IClient {
        virtual ~IClient() = 0;
        virtual grpc::Status Encrypt(TString keyId, const TVector<char>& aad,
                                     const TVector<char>& plaintext, TVector<char>& ciphertext,
                                     const TString* token = nullptr) = 0;
        virtual grpc::Status Decrypt(TString keyId, const TVector<char>& aad,
                                     const TVector<char>& ciphertext, TVector<char>& plaintext,
                                     const TString* token = nullptr) = 0;
        virtual grpc::Status GenerateDataKey(TString keyId, const TVector<char>& aad,
                                             TVector<char>& ciphertext,
                                             TVector<char>* plaintext = nullptr,
                                             const TString* token = nullptr) = 0;
    };

    using TIAMTokenProvider = std::function<TString()>;

    struct TRoundRobinClientOptions {
        TIAMTokenProvider TokenProvider;

        int MaxRetries = 10;
        TDuration InitialBackoff = TDuration::MilliSeconds(10);
        TDuration MaxBackoff = TDuration::MilliSeconds(100);
        double BackoffMultiplier = 1.5;
        double Jitter = 0.25;

        TDuration PerTryDuration = TDuration::Seconds(1);
        TDuration KeepAliveTime = TDuration::Seconds(10);

        TLog* Log = nullptr;

        bool ShuffleAddrs = true;

        TString TLSServerName;
        bool Insecure = false;
    };

    IClient* CreateRoundRobinClient(const TVector<TString>& addrs,
                                    const TRoundRobinClientOptions& options);
} // namespace NKMS
