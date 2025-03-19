#!/bin/bash

set -v

source release.sh

cd ../salt;

source ./pillar/env.sh

export REQUESTS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt
export CLI_PARAMS="--api-url ${YC_API_URL} --cloud ${YC_CLOUD} --oauth-token ${OAUTH_TOKEN} --folder ${YC_FOLDER} --identity-public-api-url $YC_IAM_URL --identity-private-api-url $YC_IAM_PRIV_URL"
export USER_DATA_FILE=/tmp/${YC_USER}-userdata.txt

if [[ "$(uname)" == Darwin ]]; then
	shopt -s expand_aliases
	alias sed=gsed
fi


prepare() {
	## Prepare
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/all/\" > /etc/apt/sources.list.d/yc-dist-yandex.list"
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud stable/amd64/\" >> /etc/apt/sources.list.d/yc-dist-yandex.list"
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/all/\" >> /etc/apt/sources.list.d/yc-dist-yandex.list"
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud testing/amd64/\" >> /etc/apt/sources.list.d/yc-dist-yandex.list"
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/all/\" >> /etc/apt/sources.list.d/yc-dist-yandex.list"
	sudo bash -c "echo \"deb http://yandex-cloud.dist.yandex.ru/yandex-cloud unstable/amd64/\" >> /etc/apt/sources.list.d/yc-dist-yandex.list"
	curl http://dist.yandex.ru/REPO.asc | sudo apt-key add -

	## we need up to date salt-ssh https://repo.saltstack.com/#ubuntu
	wget -O - https://repo.saltstack.com/apt/ubuntu/16.04/amd64/latest/SALTSTACK-GPG-KEY.pub | sudo apt-key add -
	sudo bash -c "echo \"deb http://repo.saltstack.com/apt/ubuntu/16.04/amd64/latest xenial main\" > /etc/apt/sources.list.d/saltstack.list"

	sudo apt-get update
	sudo apt-get install -y --allow-downgrades yandex-internal-root-ca salt-ssh salt-minion docker.io git yc-cli=$YC_CLI_VERSION jq tar

	cat > master <<EOF
state_verbose: True
state_output: mixed
top_file_merging_strategy: same
pillarenv_from_saltenv: True
file_roots:
  ${ENV}:
    - salt
pillar_roots:
  ${ENV}:
    - pillar/${ENV}
roster_defaults:
  user: ${YC_USER}
  sudo: True
  priv: ${YC_USER_PRIV}
EOF
}

_createInstance (){
	instance_name=`echo $1 | awk '{print $1}'`
	if [[ ! `yc-cli instance show ${instance_name} -f value -c id ${CLI_PARAMS}` ]];
	then
		image_name=`render_instance_image_name "$1"`
		image_id=$(yc-cli image show $image_name -f value -c id ${CLI_PARAMS} || echo "")

		instance_cli=`render_instance "$1" "${image_id}"`

		echo "${instance_cli}"
		yc-cli instance create ${instance_cli} --user-data ${USER_DATA_FILE} ${CLI_PARAMS}
	fi
}

deploy(){

	pub_key=$(cat pillar/${ENV}/secret/${YC_USER}.pub)
	# custom delimiter in second expr because pubkey contains backslashes
	sed "s/%user%/${YC_USER}/g;s|%pub_key%|${pub_key}|g" pillar/${ENV}/secret/userdata.txt > ${USER_DATA_FILE}

	if [[ -z "$1" ]]
	then
		for item in "${instances[@]}"
		do
			_createInstance "${item}"
		done
	else
		_createInstance "$1"
	fi

	rm ${USER_DATA_FILE}
}


fetchSecrets(){
	wget https://storage.cloud-preprod.yandex.net/skm/linux/skm -O pillar/$ENV/secret/skm
	chmod +x pillar/$ENV/secret/skm
	(
	    cd pillar/$ENV/secret
	    ./skm decrypt
	)
}

roster(){
	build_net_names="${YC_BUILD_SUBNET_A}|${YC_BUILD_SUBNET_B}|${YC_BUILD_SUBNET_C}"
	echo > roster


	for i in "${instances[@]}"; do
		node_name=`echo $i | awk '{print $1}'`
		node_info=`yc-cli instance show $node_name -f json $CLI_PARAMS`
		if [[ $node_info ]]; then
			fqdn=`echo ${node_info} | jq -r ".fqdn"`
			ip=`echo ${node_info} | jq -r ".network_interfaces[0].primaryV6Address.address"`
			build_net_id=`echo ${node_info} | grep -oE ${build_net_names} | head -n1`
			zone=`echo $i | awk '{print $5}'`
			cat >> roster << EOF
${fqdn}:
  host: ${ip}
  minion_opts:
    grains:
      YC_BUILD_SUBNET: ${build_net_id}
      YC_ZONE: ${zone}
EOF
		fi
	done
}

state(){
	chown yndx.cloud.mkt pillar/$ENV/secret/yndx.cloud.mkt
	chown -R yndx.cloud.mkt:root .
	salt-ssh --sudo -i -c . '*' state.apply saltenv=$ENV pillarenv=$ENV
}

all(){
	prepare
	deploy
	roster
	state

}

## Bootstrap
# pass step name as first arg to run one step separately
(

	case $1 in
		fetchSecrets)
			fetchSecrets
			;;

		prepare)
			prepare
			;;

		roster)
			roster
			;;

		createFolders)
			createFolders
			;;

		state)
			state
			;;

		deploy)
			# to deploy one vm run `./bootstrap.sh deploy %name% %type%`
			# `./bootstrap.sh deploy "mkt-api-1a 1 3G ipv6 ru-central1-a"`
			deploy $2
			;;

		all)
			all
			;;
	esac

)
