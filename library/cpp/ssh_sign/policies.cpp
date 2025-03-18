#include "policies.h"

#include <library/cpp/ssh/sign_agent.h>
#include <library/cpp/ssh/sign_key.h>
#include <library/cpp/ssh/ssh.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/string/builder.h>

static void CheckKeyPath(const TFsPath& filename) {
    Y_ENSURE(filename.Exists(), TStringBuilder() << "Key path does not exist: " << filename);
    //TODO(lkozhinov): Is it possible? (IsFile() && IsSymlink()) == true
    Y_ENSURE(filename.IsFile(), TStringBuilder() << "Key path is not a file: " << filename);
}

namespace NSS {
    static TStringBuf GetPublicKeyTypeId(const TStringBuf key) {
        Y_ENSURE(key.Size() >= 4, "Key is not formatted properly");

        const TStringBuf lengthBuf = key.SubString(0, 4);
        ui64 length = lengthBuf[3] | (lengthBuf[2] << 4) | (lengthBuf[1] << 8) | (lengthBuf[0] << 12);
        Y_ENSURE(4 + length <= key.size(), "Key is not formatted properly");

        return key.SubString(4, length);
    }

    static ESignType GetPublicKeyType(const TStringBuf keyType) {
        if (keyType.Contains("cert")) {
            return ESignType::CERT;
        }

        if (keyType == "ssh-rsa") {
            return ESignType::KEY;
        }

        return ESignType::UNKNOWN;
    }

    TAgentPolicy::TAgentPolicy(const TString& username)
        : Username_(username)
    {
    }

    std::optional<TResult> TAgentPolicy::SignNext(TStringBuf data) {
        try {
            if (!LazyInit()) {
                // show inition error only once
                return {};
            }

            if (CurrentPosition_ >= State_->Identities_.size()) {
                return {};
            }

            const TSSHAgentIdentityView& identity = State_->Identities_[CurrentPosition_++];
            const TString publicKey = identity.PublicKey();

            const TStringBuf typeId = GetPublicKeyTypeId(publicKey);
            const ESignType type = GetPublicKeyType(typeId);

            if (type == ESignType::UNKNOWN) {
                return TErrorMsg{"unsupported key type: " + TString(typeId)};
            }

            return TSignedData{
                .Type = type,
                .Key = publicKey,
                .Sign = State_->Agent_.Sign(identity, data, Username_),
            };
        } catch (const std::exception& e) {
            return TErrorMsg{e.what()};
        }
    }

    bool TAgentPolicy::LazyInit() {
        if (State_) {
            return true;
        }

        if (!FirstCall_) {
            return false;
        }
        FirstCall_ = false;

        State_ = std::make_unique<TState>();
        return true;
    }

    TAgentPolicy::TState::TState()
        : Session_()
        , Agent_(&Session_)
        , Identities_(Agent_.Identities())
    {
        Y_ENSURE(Identities_.size() > 0, "No identities found in agent, try running `ssh-add` first");
    }

    TCustomRsaKeyPolicy::TCustomRsaKeyPolicy(const TString& keyPath)
        : KeyPath_(keyPath)
    {
    }

    std::optional<TResult> TCustomRsaKeyPolicy::SignNext(TStringBuf data) {
        if (!FirstCall_) {
            return {};
        }
        FirstCall_ = false;

        try {
            CheckKeyPath(KeyPath_);

            return TSignedData{
                .Type = ESignType::KEY,
                .Sign = TSSHPrivateKey(KeyPath_).Sign(data),
            };
        } catch (const std::exception& e) {
            return TErrorMsg{e.what()};
        }
    }

    TStandardRsaKeyPolicy::TStandardRsaKeyPolicy()
        : TCustomRsaKeyPolicy((TFsPath{GetHomeDir()} / ".ssh" / "id_rsa").GetPath())
    {
    }
}

template <>
void Out<NSS::TErrorMsg>(IOutputStream& s, const NSS::TErrorMsg& v) {
    s << static_cast<const TString&>(v);
}
