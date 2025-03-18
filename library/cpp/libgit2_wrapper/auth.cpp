#include "auth.h"

#include <util/system/env.h>

namespace NLibgit2 {

    TCredentials::TCredentials() {
        git_cred_default_new(&Credentials_);
    }

    TCredentials::TCredentials(const TString& username) {
        git_cred_ssh_key_from_agent(&Credentials_, username.c_str());
        UserName_ = username;
    }

    TCredentials::TCredentials(const TString& username, const TString& password) {
        git_cred_userpass_plaintext_new(&Credentials_, username.c_str(), password.c_str());
        UserName_ = username;
    }

    TCredentials::TCredentials(const TString& username, const TFsPath& privateKey, const TString& passphrase) {
            git_cred_ssh_key_new(&Credentials_, username.c_str(), nullptr, privateKey.c_str(), passphrase.c_str());
            UserName_ = username;
        }

    TCredentials CredentialsFromEnvionment(const TString& /*url*/, const TString& usernameFromUrl, int /*allowedTypes*/) {
            TFsPath privateKey(GetEnv("GIT_PRIVATE_KEYFILE"));
            TString passphrase(GetEnv("GIT_PASSPHRASE"));
            return {usernameFromUrl, privateKey, passphrase };
        }

    TCredentials DefaultCredentials(const TString& /*url*/, const TString& /*usernameFromUrl*/, int /*allowedTypes*/) {
        return {};
    }

    git_cred* TCredentials::Get() {
        return Credentials_;
    }

    TString TCredentials::UserName() const {
        return UserName_;
    }

} // namespace NLibgit2
