#include "sign_agent.h"

#include "ssh.h"

TSSHAgent::TException::TException() {
    (*this) << "Ssh agent can't be used. Try check it's state with `ssh-add -l`: "
            << "it should print something like that:" << Endl
            << "    2048 SHA256:AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA username@yandex-team.ru (RSA)" << Endl
            << "The proper way to enable ssh-forwarding from your laptop to server "
            << "is add to ~/.ssh/config this options (on Linux):" << Endl
            << "Host *" << Endl
            << "ForwardAgent yes" << Endl
            << "" << Endl;
}

void TSSHAgent::TAgentDestroyer::Destroy(LIBSSH2_AGENT* agent) {
    if (agent) {
        libssh2_agent_free(agent);
    }
}

TSSHAgentIdentityView::TSSHAgentIdentityView(libssh2_agent_publickey* pub)
    : Pub_(pub)
{
}

TString TSSHAgentIdentityView::PublicKey() const {
    return {(char*) Pub_->blob, Pub_->blob_len};
}

libssh2_agent_publickey* TSSHAgentIdentityView::SSHIdentity() const {
    return Pub_;
}

TSSHAgent::TSSHAgent(TSSHThinSession* session)
    : Session_(session->GetSession())
    , Agent_(libssh2_agent_init(Session_))
    , Connector_(Agent_.Get())
{
    const int code = libssh2_agent_list_identities(Agent_.Get());
    Y_ENSURE_EX(0 == code, TException() << "libssh2 failed to list your identities. code=" << code);
}

TSmallVec<TSSHAgentIdentityView> TSSHAgent::Identities() const {
    TSmallVec<TSSHAgentIdentityView> result;

    libssh2_agent_publickey* ident = nullptr;
    struct libssh2_agent_publickey* identPrev = nullptr;
    while (libssh2_agent_get_identity(Agent_.Get(), &ident, identPrev) == 0) {
        result.emplace_back(ident);
        identPrev = ident;
    }

    return result;
}

TSmallVec<TString> TSSHAgent::Sign(const TStringBuf data, const TString& username) const {
    TSmallVec<TString> result;

    struct libssh2_agent_publickey* ident = nullptr;
    struct libssh2_agent_publickey* identPrev = nullptr;
    while (libssh2_agent_get_identity(Agent_.Get(), &ident, identPrev) == 0) {
        result.push_back(SignIdentity(ident, data, username));
        identPrev = ident;
    }

    return result;
}

TString TSSHAgent::Sign(const TSSHAgentIdentityView& ident, const TStringBuf data, const TString& username) const {
    return SignIdentity(ident.SSHIdentity(), data, username);
}

TString TSSHAgent::SignIdentity(libssh2_agent_publickey* ident, const TStringBuf data, const TString& username) const {
    libssh2_agent_userauth(Agent_.Get(), username.data(), ident);

    auto libssh2Free = [&](unsigned char* sig) {
        if (sig) {
            libssh2_free(Session_, sig);
        }
    };
    std::unique_ptr<unsigned char, decltype(libssh2Free)> sigPtr(nullptr, libssh2Free);
    unsigned char* sig = nullptr;

    size_t len = 0;

    const int code = _ya_libssh2_agent_sign(Session_,
                                            &sig,
                                            &len,
                                            (unsigned char*)data.data(),
                                            data.size(),
                                            Agent_.Get());
    sigPtr.reset(sig);
    Y_ENSURE_EX(0 == code, TException() << "libssh2 failed to sign your data. code=" << code);

    return {(char*)sig, len};
}

int TSSHAgent::AuthorizeSession(const TString& username) {
    int rc;
    struct libssh2_agent_publickey *identity, *prev_identity = NULL;
    while (true) {
        rc = libssh2_agent_get_identity(Agent_.Get(), &identity, prev_identity);

        if (rc == 1) {
            // All private keys have already been tried
            break;
        } else if (rc < 0) {
            // Error while handling key identity
            char* buffer = nullptr;
            libssh2_session_last_error(Session_, &buffer, nullptr, 0);
            ythrow yexception() << "Error while handling key via ssh-agent: " << buffer << Endl;
        }

        if (!libssh2_agent_userauth(Agent_.Get(), username.c_str(), identity)) {
            // Authrozation succeed
            break;
        }
        prev_identity = identity;
    }
    return rc;
}

TSSHAgent::TConnector::TConnector(LIBSSH2_AGENT* agent)
    : Agent_(agent)
{
    Y_ENSURE(Agent_);
    const int code = libssh2_agent_connect(Agent_);
    Y_ENSURE_EX(0 == code, TException() << "libssh2 failed to connect to ssh-agent socket. code=" << code);
}

TSSHAgent::TConnector::~TConnector() {
    libssh2_agent_disconnect(Agent_);
}
