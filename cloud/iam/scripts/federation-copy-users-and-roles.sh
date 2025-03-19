#!/bin/bash
#set -x
set -e

FEDERATION_ID=$1
OLD_DOMAIN=$2
NEW_DOMAIN=$3
PROFILE=${4:-default}

if [[ -z "${NEW_DOMAIN}" ]]; then
  echo "Usage: $0 <FEDERATION_ID> <OLD_DOMAIN> <NEW_DOMAIN> [profile=default]"
  exit
fi

function resolve-user() {
  federation_id=$1
  name_id=$2

  yc --profile=${PROFILE} --format json --no-user-output \
    organization-manager federation saml add-user-accounts --id ${federation_id} --name-ids=${name_id} |
    jq -rc ".user_accounts[] | {name_id: .saml_user_account.name_id, id: .id}"
}

function grant_role() {

  case $1 in

    organization-manager.organization)
    resource_type=organization
    service=organization-manager
    ;;

    resource-manager.cloud)
    resource_type=cloud
    service=resource-manager
    ;;

    resource-manager.folder)
    resource_type=folder
    service=resource-manager
    ;;

    iam.serviceAccount)
    resource_type=service-account
    service=iam
    ;;

    *)
    echo "Unsupported resource type. Please manually grant to user $4 role $3 on resource $1:$2"
    return 0
    ;;

  esac

  resource_id=$2
  role=$3
  subject_id=$4
  yc --profile=${PROFILE} --format=json --no-user-output \
    ${service?} ${resource_type?} --id  ${resource_id} \
    add-access-binding --role=${role} --subject=federatedUser:${subject_id} || true
}

while read -r line; do
  name_id=$(jq -rc .user.name_id <<< ${line})
  new_name_id=$(sed -e "s/@${OLD_DOMAIN}/@${NEW_DOMAIN}/g" <<< ${name_id})

  new_user=$(resolve-user ${FEDERATION_ID} ${new_name_id})
  new_user_id=$(jq -rc .id <<< ${new_user})
  
  echo "$name_id => $new_name_id ($new_user_id)"

  bindings=$(jq -rc ".bindings | .[] | \"\(.resource.type):\(.resource.id):\(.role_id)\"" <<< ${line})

  for binding in $bindings
  do
    grant_role $(tr : ' ' <<< $binding) ${new_user_id}
  done

done
