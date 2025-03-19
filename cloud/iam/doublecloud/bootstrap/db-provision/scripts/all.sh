#!/bin/bash
set -e

RMCP_URL="localhost"
IAMCP_URL="localhost"
ORG_URL="localhost"

./iam-yandex-team-federation ${ORG_URL?}

./iam-service-cloud.sh ${RMCP_URL?}

./iam-service-resources.sh accessService access-service "Access Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh controlPlane control-plane "Control Plane Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh mfaService mfa-service "MFA Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh openidServer openid-server "OpenID Server" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh orgService org-service "Organization Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh reaper reaper "Reaper Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh rmControlPlane rm-control-plane "Resource Manager Control Plane Service" ${RMCP_URL?} ${IAMCP_URL?}
./iam-service-resources.sh tokenService token-service "Token Service" ${RMCP_URL?} ${IAMCP_URL?}

./iam-sync.sh ${IAMCP_URL?}

./grant-roles.sh ${IAMCP_URL?}
