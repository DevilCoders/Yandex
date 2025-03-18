#pragma once

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/yexception.h>

struct _LIBSSH2_AGENT;
typedef struct _LIBSSH2_AGENT LIBSSH2_AGENT;

struct _LIBSSH2_SESSION;
typedef struct _LIBSSH2_SESSION LIBSSH2_SESSION;

struct libssh2_agent_publickey;

class TSSHThinSession;

class TSSHAgentIdentityView {
public:
    explicit TSSHAgentIdentityView(libssh2_agent_publickey* pub);
    TString PublicKey() const;
    libssh2_agent_publickey* SSHIdentity() const;

private:
    libssh2_agent_publickey* Pub_;
};

class TSSHAgent {
public:
    class TException: public yexception {
    public:
        TException();
    };

public:
    explicit TSSHAgent(TSSHThinSession* session);

    // Returns all available identities from ssh-agent
    TSmallVec<TSSHAgentIdentityView> Identities() const;

    // Sign data with all available identities
    TSmallVec<TString> Sign(const TStringBuf data, const TString& username) const;

    // Sign data with provided identity
    TString Sign(const TSSHAgentIdentityView& ident, const TStringBuf data, const TString& username) const;

    /* Authorize a session using a previously launched ssh-agent
      *
      * @return 0, if session session was authorized
      *         1, otherwise
      * @throw  yexception if any internal error occurred
      */
    int AuthorizeSession(const TString& username);

private:
    TString SignIdentity(libssh2_agent_publickey* ident, const TStringBuf data, const TString& username) const;

private:
    class TAgentDestroyer {
    public:
        static void Destroy(LIBSSH2_AGENT* agent);
    };

    class TConnector {
        LIBSSH2_AGENT* Agent_;

    public:
        TConnector(LIBSSH2_AGENT* agent);
        ~TConnector();
    };

private:
    LIBSSH2_SESSION* Session_;
    THolder<LIBSSH2_AGENT, TAgentDestroyer> Agent_;
    TConnector Connector_;
};
