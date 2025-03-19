#!/bin/bash
set -e

IAMCP_URL=$1

if [[ -z ${IAMCP_URL} ]]; then
    IAMCP_URL=iam.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${IAMCP_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL?})


# TODO
echo Grant organization-manager.admin for eve
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "eve" "organization-manager.admin" "organization-manager.organization" "yc.organization-manager.yandex" "${IAMCP_URL?}" | jq

####################
echo Grant roles for yc.iam.controlPlane SA
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.controlPlane" "internal.iam.agent" "${IAMCP_URL?}" | jq


####################
echo Grant roles for yc.iam.rmControlPlane SA
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.rmControlPlane" "internal.resource-manager.agent" "${IAMCP_URL?}" | jq

## TODO Remove it/ Temporary grand viewer to root
#internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.controlPlane" "viewer" "${IAMCP_URL?}" | jq
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.rmControlPlane" "internal.iam.listResourceTypeMemberships" "iam.resourceType" "organization-manager.organization" "${IAMCP_URL?}" | jq

internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.rmControlPlane" "iam.admin" "iam.resourceType" "organization-manager.organization" "${IAMCP_URL?}" | jq
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.rmControlPlane" "internal.iam.accessBindings.admin" "iam.resourceType" "resource-manager.cloud" "${IAMCP_URL?}" | jq
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.rmControlPlane" "internal.iam.accessBindings.admin" "iam.resourceType" "resource-manager.folder" "${IAMCP_URL?}" | jq


####################
echo Grant roles for yc.iam.openidServer SA
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.openidServer" "internal.sessionService" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.openidServer" "internal.saas.agent" "${IAMCP_URL?}" | jq

internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.openidServer" "kms.keys.encrypterDecrypter" "resource-manager.folder" "yc.iam.service" "${IAMCP_URL?}" | jq


####################
echo Grant roles for yc.iam.orgService SA
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.orgService" "iam.admin" "iam.resourceType" "organization-manager.organization" "${IAMCP_URL?}" | jq
internal/grant-resource-type-role.sh "${IAM_TOKEN?}" "yc.iam.orgService" "internal.iam.listResourceTypeMemberships" "iam.resourceType" "organization-manager.organization" "${IAMCP_URL?}" | jq

internal/grant-gizmo-role.sh "${IAM_TOKEN?}" "yc.iam.orgService" "internal.organization-manager.agent" "${IAMCP_URL?}" | jq


####################
echo Grant roles for yc.iam.sync SA
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "internal.iam.sync" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "internal.resource-manager.sync" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "iam.admin" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "internal.iam.resourceTypes.admin" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "internal.iam.crossCloudBindings" "${IAMCP_URL?}" | jq
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "internal.billing.admin" "${IAMCP_URL?}" | jq
# TODO
internal/grant-root-role.sh "${IAM_TOKEN?}" "yc.iam.sync" "organization-manager.editor" "${IAMCP_URL?}" | jq
