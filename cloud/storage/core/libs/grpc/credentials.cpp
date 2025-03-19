#include "credentials.h"

#include <contrib/libs/grpc/include/grpc/support/alloc.h>
#include <contrib/libs/grpc/src/core/lib/security/context/security_context.h>
#include <contrib/libs/grpc/src/core/lib/security/credentials/credentials.h>
#include <contrib/libs/grpc/src/core/lib/security/security_connector/security_connector.h>
#include <contrib/libs/grpc/src/core/lib/security/transport/security_handshaker.h>
#include <contrib/libs/grpc/src/core/tsi/local_transport_security.h>
#include <contrib/libs/grpc/src/cpp/server/secure_server_credentials.h>

#include <util/system/compiler.h>
#include <util/system/yassert.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TInsecureConnector final
    : public grpc_server_security_connector
{
public:
    TInsecureConnector(grpc_core::RefCountedPtr<grpc_server_credentials> creds)
        : grpc_server_security_connector(nullptr, std::move(creds))
    {}

    void check_peer(
        tsi_peer peer,
        grpc_endpoint* endpoint,
        grpc_core::RefCountedPtr<grpc_auth_context>* authContext,
        grpc_closure* onPeerChecked) override
    {
        Y_UNUSED(endpoint);

        *authContext = grpc_core::MakeRefCounted<grpc_auth_context>(nullptr);
        grpc_error* error = GRPC_ERROR_NONE;
        grpc_core::ExecCtx::Run(DEBUG_LOCATION, onPeerChecked, error);
        tsi_peer_destruct(&peer);
    }

    void add_handshakers(
        const grpc_channel_args* args,
        grpc_pollset_set* interestedParties,
        grpc_core::HandshakeManager* handshakeManager) override
    {
        Y_UNUSED(interestedParties);

        // Creating handshaker for local connections. It does not actually do
        // any handshaking, but grpc internal code requires some handshaker to
        // be present.
        tsi_handshaker* handshaker = nullptr;
        bool isClient = false;
        auto handshakerResult =
            tsi_local_handshaker_create(isClient, &handshaker);
        if (handshakerResult != TSI_OK) {
            gpr_log(GPR_ERROR, "Could not create grpc handshaker %s",
                tsi_result_to_string(handshakerResult));
            return;
        }

        handshakeManager->Add(
            grpc_core::SecurityHandshakerCreate(handshaker, this, args));
    }

    int cmp(const grpc_security_connector* other) const override
    {
        return server_security_connector_cmp(
            static_cast<const grpc_server_security_connector*>(other));
    }

    void cancel_check_peer(grpc_closure* /*on_peer_checked*/,
                           grpc_error_handle error) override {
      GRPC_ERROR_UNREF(error);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TInsecureServerCredentials final
    : public grpc_server_credentials
{
public:
    TInsecureServerCredentials()
        : grpc_server_credentials("InsecureServerCredentials")
    {}

    grpc_core::RefCountedPtr<grpc_server_security_connector>
    create_security_connector(const grpc_channel_args* /*args*/) override
    {
        return grpc_core::MakeRefCounted<TInsecureConnector>(Ref());
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<grpc::ServerCredentials> CreateInsecureServerCredentials()
{
    return std::make_shared<grpc::SecureServerCredentials>(
        new TInsecureServerCredentials());
}

}   // namespace NCloud
