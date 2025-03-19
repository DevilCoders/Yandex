#include "kmsclient.h"

#include <cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1/symmetric_crypto_service.grpc.pb.h>

#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>
#include <library/cpp/deprecated/atomic/atomic.h>

#include <memory>
#include <chrono>

// Doing filtration level checks at log caller level is faster due to branch prediction.
#define KMS_DEBUG(...)                                            \
    if (Log != nullptr && Log->FiltrationLevel() >= TLOG_DEBUG) { \
        Log->AddLog(TLOG_DEBUG, __VA_ARGS__);                     \
    }

#define KMS_TRACE(...)                                                \
    if (Log != nullptr && Log->FiltrationLevel() >= TLOG_RESOURCES) { \
        Log->AddLog(TLOG_RESOURCES, __VA_ARGS__);                     \
    }

namespace NKMS {
    namespace kms = yandex::cloud::priv::kms::v1;

    IClient::~IClient() {
    }

    class TRoundRobinClient : public IClient {
    public:
        TRoundRobinClient(const TVector<TString>& addrs, const TRoundRobinClientOptions& options);
        ~TRoundRobinClient();

        grpc::Status Encrypt(TString keyId, const TVector<char>& aad,
                             const TVector<char>& plaintext, TVector<char>& ciphertext,
                             const TString* token) override;
        grpc::Status Decrypt(TString keyId, const TVector<char>& aad,
                             const TVector<char>& ciphertext, TVector<char>& plaintext,
                             const TString* token) override;
        grpc::Status GenerateDataKey(TString keyId, const TVector<char>& aad,
                                     TVector<char>& ciphertext,
                                     TVector<char>* plaintext,
                                     const TString* token) override;

    private:
        TVector<TString> Addrs;
        TVector<std::shared_ptr<grpc::Channel>> Channels;
        TVector<std::unique_ptr<kms::SymmetricCryptoService::Stub>> Stubs;
        TAtomic LastIndex;

        TIAMTokenProvider TokenProvider;

        int MaxRetries;
        TDuration InitialBackoff;
        TDuration MaxBackoff;
        float BackoffMultiplier;
        float Jitter;

        TDuration PerTryDuration;
        TDuration KeepAliveTime;

        TLog* Log;

    private:
        size_t GetAndIncrementLastIndex();

        void InitContext(const TString* token, grpc::ClientContext& context) const;

        bool IsRetryable(const grpc::Status& status) const;

        float JitterFactor() const;

        template <typename F>
        grpc::Status DoCall(F&& call);
    };

    TRoundRobinClient::TRoundRobinClient(const TVector<TString>& addrs, const TRoundRobinClientOptions& options)
        : Addrs(addrs)
        , LastIndex(0)
        , TokenProvider(options.TokenProvider)
        , MaxRetries(options.MaxRetries)
        , InitialBackoff(options.InitialBackoff)
        , MaxBackoff(options.MaxBackoff)
        , BackoffMultiplier(options.BackoffMultiplier)
        , Jitter(options.Jitter)
        , PerTryDuration(options.PerTryDuration)
        , KeepAliveTime(options.KeepAliveTime)
        , Log(options.Log) {
        Y_VERIFY(!addrs.empty(), "at least one addr is required");

        if (options.ShuffleAddrs) {
            // Shuffle addrs so that different clients will use different backend orders.
            Shuffle(Addrs.begin(), Addrs.end());
        }

        grpc::ChannelArguments channelArgs;
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, KeepAliveTime.MilliSeconds());
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, PerTryDuration.MilliSeconds());
        if (!options.TLSServerName.empty()) {
            channelArgs.SetString(GRPC_SSL_TARGET_NAME_OVERRIDE_ARG, options.TLSServerName);
        }
        for (const auto& addr : Addrs) {
            KMS_DEBUG("opening channel to %s\n", addr.c_str());
            auto creds = options.Insecure ? grpc::InsecureChannelCredentials()
                                          : grpc::SslCredentials(grpc::SslCredentialsOptions());
            auto channel = grpc::CreateCustomChannel(addr, creds, channelArgs);
            Channels.emplace_back(channel);
            Stubs.emplace_back(new kms::SymmetricCryptoService::Stub(channel));
        }
    }

    TRoundRobinClient::~TRoundRobinClient() {
    }

    grpc::Status TRoundRobinClient::Encrypt(TString keyId, const TVector<char>& aad,
                                            const TVector<char>& plaintext, TVector<char>& ciphertext,
                                            const TString* token) {
        kms::SymmetricEncryptRequest request;
        request.set_key_id(keyId);
        request.set_aad_context(aad.data(), aad.size());
        request.set_plaintext(plaintext.data(), plaintext.size());
        ciphertext.clear();

        return DoCall([&](kms::SymmetricCryptoService::Stub* stub) {
            grpc::ClientContext context;
            InitContext(token, context);
            kms::SymmetricEncryptResponse response;
            grpc::Status st = stub->Encrypt(&context, request, &response);
            if (st.ok()) {
                ciphertext.assign(response.ciphertext().begin(), response.ciphertext().end());
            }
            return st;
        });
    }

    grpc::Status TRoundRobinClient::Decrypt(TString keyId, const TVector<char>& aad,
                                            const TVector<char>& ciphertext, TVector<char>& plaintext,
                                            const TString* token) {
        grpc::ClientContext context;
        kms::SymmetricDecryptRequest request;
        request.set_key_id(keyId);
        request.set_aad_context(aad.data(), aad.size());
        request.set_ciphertext(ciphertext.data(), ciphertext.size());
        plaintext.clear();

        return DoCall([&](kms::SymmetricCryptoService::Stub* stub) {
            grpc::ClientContext context;
            InitContext(token, context);
            kms::SymmetricDecryptResponse response;
            grpc::Status status = stub->Decrypt(&context, request, &response);
            if (status.ok()) {
                plaintext.assign(response.plaintext().begin(), response.plaintext().end());
            }
            auto st = status;
            return status;
        });
    }

    grpc::Status TRoundRobinClient::GenerateDataKey(TString keyId, const TVector<char>& aad,
                                                    TVector<char>& ciphertext,
                                                    TVector<char>* plaintext,
                                                    const TString* token) {
        grpc::ClientContext context;
        kms::GenerateDataKeyRequest request;
        request.set_key_id(keyId);
        request.set_aad_context(aad.data(), aad.size());
        request.set_skip_plaintext(plaintext == nullptr);
        ciphertext.clear();

        if (plaintext) {
            plaintext->clear();
        }

        return DoCall([&](kms::SymmetricCryptoService::Stub* stub) {
            grpc::ClientContext context;
            InitContext(token, context);
            kms::GenerateDataKeyResponse response;
            grpc::Status status = stub->GenerateDataKey(&context, request, &response);
            if (status.ok()) {
                ciphertext.assign(
                    response.data_key_ciphertext().begin(),
                    response.data_key_ciphertext().end());
                if (plaintext) {
                    plaintext->assign(
                        response.data_key_plaintext().begin(),
                        response.data_key_plaintext().end());
                }
            }
            auto st = status;
            return status;
        });
    }

    size_t TRoundRobinClient::GetAndIncrementLastIndex() {
        while (true) {
            size_t ret = AtomicGet(LastIndex);
            size_t next = (ret + 1) % Channels.size();
            if (AtomicCas(&LastIndex, next, ret)) {
                return ret;
            }
        }
    }

    void TRoundRobinClient::InitContext(const TString* token, grpc::ClientContext& context) const {
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(PerTryDuration.MilliSeconds()));
        TString iamToken = (token != nullptr) ? *token : TokenProvider();
        context.AddMetadata("authorization", "Bearer " + iamToken);
    }

    bool TRoundRobinClient::IsRetryable(const grpc::Status& status) const {
        switch (status.error_code()) {
            case grpc::DEADLINE_EXCEEDED:
            case grpc::RESOURCE_EXHAUSTED:
            case grpc::UNAVAILABLE:
                return true;
            default:
                return false;
        }
    }

    float TRoundRobinClient::JitterFactor() const {
        return 1.0f - RandomNumber<float>() * Jitter;
    }

    template <typename F>
    grpc::Status TRoundRobinClient::DoCall(F&& call) {
        size_t stubIdx = GetAndIncrementLastIndex();
        bool onlyReady = true;
        ui64 nextSleepNs = InitialBackoff.NanoSeconds();
        ui64 maxSleepNs = MaxBackoff.NanoSeconds();
        for (int numRetry = 0; numRetry < MaxRetries + 1; numRetry++) {
            if (onlyReady) {
                // Find the first READY channel. Try connecting the IDLE channels along the way.
                size_t skipped = 0;
                for (skipped = 0; skipped < Channels.size(); skipped++) {
                    grpc_connectivity_state state = Channels[stubIdx]->GetState(true);
                    if (state == GRPC_CHANNEL_READY) {
                        break;
                    }
                    KMS_TRACE("backend #%d (%s) in state %d, skipping\n", (int)stubIdx,
                              Addrs[stubIdx].c_str(), state);
                    stubIdx++;
                    if (stubIdx >= Channels.size()) {
                        stubIdx = 0;
                    }
                }
                if (skipped == Channels.size()) {
                    // All channels are not read, do not try searching for READY channel.
                    onlyReady = false;
                }
            }

            grpc::Status st = call(Stubs[stubIdx].get());
            if (!IsRetryable(st) || numRetry == MaxRetries) {
                return st;
            }

            ui64 ns = nextSleepNs * JitterFactor();
            KMS_DEBUG("got status %d (%s) on retry %d (backend #%d (%s)), retrying in %dms\n",
                      st.error_code(), st.error_message().c_str(), numRetry, (int)stubIdx,
                      Addrs[stubIdx].c_str(), int(ns / 1000000));
            NanoSleep(ns);
            nextSleepNs = Min(ui64(nextSleepNs * BackoffMultiplier), maxSleepNs);
            stubIdx++;
            if (stubIdx >= Channels.size()) {
                stubIdx = 0;
            }
        }
        // We iterate until MaxRetries + 1.
        Y_UNREACHABLE();
    }

    IClient* CreateRoundRobinClient(const TVector<TString>& addrs, const TRoundRobinClientOptions& options) {
        return new TRoundRobinClient(addrs, options);
    }
} // namespace NKMS
