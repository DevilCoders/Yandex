#!/usr/bin/env bash

YDB_TOKEN="${YDB_TOKEN:-$(cat ~/.config/kiksaint/token)}"
YDB_PREFIX="/global"
KIKHOUSE_HOST="https://kikhouse.svc.kikhouse.bastion.cloud.yandex-team.ru"
ENV="prod"

_ydbTable()
{
	local cluster="ycloud"
	local database="${YDB_PREFIX}/${1}"
	local table="${2}"

	case "${ENV}" in
		preprod)
			echo "ydbTable('${database}/${table}')"
			#echo "ydbTable('${cluster}', '${database}', '${table}')"
			;;
		prod)
			#echo "ydbTable('${database}/${table}')"
			echo "ydbTable('${cluster}', '${database}', '${table}')"
			;;
	esac
}


_set_env()
{
	local env="${1}"
	case "$env" in
		preprod)
			KIKHOUSE_HOST="https://kikhouse.svc.kikhouse-preprod.bastion.cloud.yandex-team.ru"
			YDB_PREFIX="/pre-prod_global"
			;;
		prod)
			KIKHOUSE_HOST="https://kikhouse.svc.kikhouse.bastion.cloud.yandex-team.ru"
			YDB_PREFIX="/global"
			;;
	esac
	ENV="${env}"
}

_run_nlbip2inst()
{
    local address="${1}"
    local fields="${2:-id, cloud_id, compute_node, name}"
	local query
	query=\
"SELECT ${fields}
  FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/instances')
 WHERE id IN (
    SELECT instance_id
    FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_interface_attachments')
    WHERE (subnet_id, JSONExtractString(data, 'ipv6_address')) IN (
        SELECT
            subnet_id,
            address
        FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/target_group_targets')
        WHERE target_group_id IN (
            SELECT target_group_id
            FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_load_balancer_attachments')
            WHERE network_load_balancer_id IN (
            	SELECT network_load_balancer_id
            	FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/addresses')
            	WHERE address = '${address}'
            )
        )
    )
 )
 FORMAT PrettyCompactMonoBlock
 ;"
	echo "$query"
}

_run_nlb2inst()
{
    local id="${1}"
    local fields="${2:-id, cloud_id, compute_node, name}" 
    local query=\
"SELECT ${fields}
  FROM ydbTable('ycloud' 'hardware/default/compute_az/instances')
 WHERE id IN (
    SELECT instance_id
    FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_interface_attachments')
    WHERE (subnet_id, JSONExtractString(data, 'ip_address')) IN (
        SELECT
            subnet_id,
            address
        FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/target_group_targets')
        WHERE target_group_id IN (
            SELECT target_group_id
            FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_load_balancer_attachments')
            WHERE network_load_balancer_id = '${id}'
        )
    )
 )
 FORMAT PrettyCompactMonoBlock
 ;"
    echo "$query"
}

_run_ip62inst()
{
    local address="${1}"
    local fields="${2:-id, folder_id, cloud_id, compute_node, name}" 
    local query=\
"SELECT ${fields}
  FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/instances')
 WHERE id IN (
    SELECT instance_id
    FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_interface_attachments')
    WHERE JSONExtractString(data, 'ipv6_address') = '${address}'
 )
 FORMAT PrettyCompactMonoBlock
 ;"
    echo "$query"
}

_run_host2nlb()
{
    local host="${1}"
    local address="$(getent hosts ${host} | cut -f1 -d' ')"
    local fields="${2:-id, folder_id, cloud_id, name}" 
	local query=\
"SELECT *
   FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/network_load_balancer_listeners')
  WHERE network_load_balancer_id IN (
    SELECT network_load_balancer_id
    FROM $(_ydbTable 'ycloud' 'hardware/default/compute_az/addresses')
    WHERE address = '${address}'
  )
 FORMAT PrettyCompactMonoBlock
 ;"
    echo "$query"	
}

_run_nlbhost2inst()
{
    local host="${1}"
    local address="$(getent hosts ${host} | cut -f1 -d' ')"
    _run_nlbip2inst "${address}"
}

_run_ytuid()
{
	local staff_login="${1}"
	local query=\
"
SELECT id, name_id, federation_id
  FROM $(_ydbTable 'iam' 'hardware/default/identity/r3/subjects/saml_federated_user_references')
 WHERE federation_id='yc.yandex-team.federation' AND name_id='${staff_login}@yandex-team.ru'
FORMAT PrettyCompactMonoBlock;
"
	echo "${query}"
}

_run_bindings()
{
	local subject_id="${1}"
	local query=\
"
SELECT *
  FROM $(_ydbTable 'iam' 'hardware/default/identity/r3/role_bindings_subject_index')
 WHERE subject_id = '${subject_id}'
FORMAT PrettyCompactMonoBlock;
"
	echo "${query}"
}

_run_crt()
{
	local cert_id="${1}"
	local query=\
"
SELECT id, status_url, crt_api_id
  FROM $(_ydbTable 'certificate-manager' 'crt_api_request')
 WHERE id = '${cert_id}'
FORMAT PrettyCompactMonoBlock;
"
	echo "${query}"
}


_send_req() {
    local curl_query="${1}"
    curl -k -H "Authorization: OAuth ${YDB_TOKEN}" "${KIKHOUSE_HOST}" \
    -d "${curl_query}"
}

_main()
{
	local env="${1}"
    local cmd="${2}"
    local opts="${3}"
    _set_env "${env}"
    local query="$(eval \"_run_${cmd}\" ${opts})"
    _send_req "${query}"
}

_main "$@"
