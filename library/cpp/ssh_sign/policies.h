#pragma once

#include <library/cpp/ssh/sign_agent.h>
#include <library/cpp/ssh/ssh.h>

#include <util/generic/string.h>

#include <optional>
#include <variant>

namespace NSS {
    enum class ESignType {
        CERT = 0,
        KEY = 1,
        UNKNOWN = 2,
    };

    struct TSignedData {
        ESignType Type = ESignType::UNKNOWN;
        TString Key;
        TString Sign;
    };

    class TErrorMsg: public TString {
    public:
        template <class... TArgs>
        TErrorMsg(TArgs&&... args)
            : TString(std::forward<TArgs>(args)...)
        {
        }
    };

    using TResult = std::variant<TSignedData, TErrorMsg>;

    class ILazySigner {
    public:
        virtual ~ILazySigner() = default;

        virtual std::optional<TResult> SignNext(TStringBuf data) = 0;
    };
    using TLazySignerPtr = std::unique_ptr<ILazySigner>;

    class TAgentPolicy: public ILazySigner {
    public:
        TAgentPolicy(const TString& username);

        std::optional<TResult> SignNext(TStringBuf data) override;

    private:
        struct TState {
            TState();

            TSSHThinSession Session_;
            TSSHAgent Agent_;
            TStackVec<TSSHAgentIdentityView> Identities_;
        };

    private:
        bool LazyInit();

    private:
        TString Username_;
        std::unique_ptr<TState> State_;
        size_t CurrentPosition_ = 0;
        bool FirstCall_ = true;
    };

    class TCustomRsaKeyPolicy: public ILazySigner {
    public:
        TCustomRsaKeyPolicy(const TString& keyPath);

        std::optional<TResult> SignNext(TStringBuf data) override;

    private:
        TString KeyPath_;
        bool FirstCall_ = true;
    };

    class TStandardRsaKeyPolicy: public TCustomRsaKeyPolicy {
    public:
        TStandardRsaKeyPolicy();
    };
}
