#!/bin/bash

YC_API_URL='https://iaas.private-api.cloud.yandex.net'
YC_IAM_URL='https://identity.private-api.cloud.yandex.net:14336'
YC_IAM_PRIV_URL='https://identity.private-api.cloud.yandex.net:14336'
YC_CLOUD='ycmarketplace'
YC_FOLDER='mkt-prod'
YC_DEFAULT_FOLDER_ID="b1g9dapghac3s4tkhv24"

YC_USER='yndx.cloud.mkt'
YC_IAM_USER='yndx-cloud-mkt'
YC_USER_PRIV="pillar/prod/secret/${YC_USER}"

YC_NETWORK_INTERFACE_IPV4='cloudmktprodnetsv4'
YC_NETWORK_INTERFACE_IPV6='cloudmktprodnets'

YC_BUILD_SUBNET_A='e9blndfa5f3hnqjkpm6e'
YC_BUILD_SUBNET_B='e2l6839igiehu5t5930b'
YC_BUILD_SUBNET_C='b0ck7ijd6f7p896o361g'

render_network() {
	net_template=`echo $1 | awk '{print $4}'`
	zone_id=`echo $1 | awk '{print $5}'`
	result=""
	while IFS=";" read -ra nets; do
		for val in "${nets[@]}"; do
			result+=" --network-interface "
			case $val in
				ipv4) # ipv4 network with public ip
					result+="subnet_id="${YC_NETWORK_INTERFACE_IPV4}"-"$zone_id",primary_v4_address_spec.nat_spec.ip_version=ipv4"
					;;
				ipv6) # ipv6 network without public ip
					result+="subnet_id="${YC_NETWORK_INTERFACE_IPV6}"-"$zone_id",primary_v6_address_spec=,primary_v4_address_spec="
					;;
				build) # ipv4 network without public ip (subnet_id from env `YC_BUILD_SUBNET_{zone suffix}`)
					build_net_id=YC_BUILD_SUBNET_`echo ${zone_id: -1} | awk '{print toupper($0)}'`
					result+="subnet_id="${!build_net_id}",primary_v4_address_spec="
					;;
				*)
					result+=$val
					;;
			esac
		done
	done <<< "$net_template"

	echo $result
}

render_boot() {
	echo $1 | awk -v img=$2 '{print "--boot-disk disk_spec.size="$7",disk_spec.image_id="img",disk_spec.type_id="$8}'
}

render_res() {
	echo $1 | awk '{print "--resources cores="$2",memory="$3" --platform-id standard-v1"}'
}

render_instance() {
	network_str=`render_network "$1"`
	boot_str=`render_boot "$1" "$2"`
	res_str=`render_res "$1"`
	echo $1 | awk -v net="$network_str" -v boot="$boot_str" -v res="$res_str" '{print "--name "$1" --fqdn "$1".ru-central1.internal "res" "net" "boot" --zone-id "$5}'
}

render_instance_image_name(){
	echo $1 | awk '{print $6}'
}

declare -a instances=(
	"mkt-api-4a 4 8G ipv6 ru-central1-a mkt-base-16 32212254720 network-hdd"
	"mkt-api-5a 4 8G ipv6 ru-central1-a mkt-base-16 32212254720 network-hdd"
	"mkt-api-4b 4 8G ipv6 ru-central1-b mkt-base-16 32212254720 network-hdd"
	"mkt-api-5b 4 8G ipv6 ru-central1-b mkt-base-16 32212254720 network-hdd"
	"mkt-api-4c 4 8G ipv6 ru-central1-c mkt-base-16 32212254720 network-hdd"
	"mkt-api-5c 4 8G ipv6 ru-central1-c mkt-base-16 32212254720 network-hdd"
	"mkt-queue-1c 1 3G ipv6 ru-central1-c mkt-base-16 32212254720 network-hdd"
	"mkt-queue-1b 1 3G ipv6 ru-central1-b mkt-base-16 32212254720 network-hdd"
	"mkt-factory-1a 4 16G ipv6;build ru-central1-a mktbase-factory 32212254720 network-hdd"
	"mkt-factory-1b 4 16G ipv6;build ru-central1-b mktbase-factory 32212254720 network-hdd"
)

render_image_params(){
	echo $1 | awk '{print "--uri "$1" --name "$2}'
}

render_image_name_param(){
	echo $1 | awk '{print $2}'
}

declare -a base_images=(
	"https://yc-bootstrap.s3.mds.yandex.net/mkt-base-img-16.04 mkt-base-16"
	"https://yc-bootstrap.s3.mds.yandex.net/mkt-base-img-18.04 mkt-base-18"
)


YC_ADDRESS_TYPE='--ipv6 --internal'

EXTRAROLES_FOR_FOLDER="standard-images"
