#!/bin/bash

ENVIRONMENTS="cloudvm hw-lab testing preprod prod private-testing private-gpn-1"
# Some environments required smooth certificates update process:
# 1. New certificates are generated
# 2. These certificates are added to current Cassandra truststore
# 3. Cassandra truststore is deploying everywhere
# 4. New certificates is deploying everywhere
# 5. New truststore without old certificates is generating
# 6. New truststore is deploying everywhere
#
# This process needs two releases, we use it for "production" environments such as testing, preprod and prod,
# and don't use it for cloudvm and hw-labs.
#
# For step 5 (generating fully new truststore) set
# ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE=""
ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE="testing preprod prod private-testing private-gpn-1"

# Calculate the diff between two lists
ENVIRONMENTS_GENERATE_NEW_CASSANDRA_TRUSTSTORE=`comm -23 <(echo $ENVIRONMENTS | tr ' ' '\n' | sort) <(echo $ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE | tr ' ' '\n' | sort)`

DATACENTERS="vla sas myt"
PRIVATE_TESTING_DATACENTERS="sas"
PRIVATE_GPN_DATACENTERS="lnx"

CASSANDRA_PASSWD_DIR="/tmp/cassandra-passwds"
CASSANDRA_TRUSTSTORE_DIR="/tmp/cassandra-truststores"
RABBIT_PASSWD_DIR="/tmp/rabbit-passwds"
COOKIE_DIR="/tmp/rabbitmq-cookies"

YC_ISSUE_CERT_TOOL="./src/yc-issue-certs"

get_dc_list() {
    local env="$1"
    if [[ $env =~ ^(testing|preprod|prod)$ ]] ; then
        echo $DATACENTERS
    elif [[ $env == "private-testing" ]] ; then
        echo $PRIVATE_TESTING_DATACENTERS
    elif [[ $env == "private-gpn-1" ]] ; then
        echo $PRIVATE_GPN_DATACENTERS
    else
        echo "all"
    fi
}

get_conductor_group() {
    local env="$1"
    local dc="$2"

    local conductor_env=$env
    # Just a special case for private-gpn-1
    if [[ $env == "private-gpn-1" ]] ; then
        conductor_env="private-gpn_private-gpn-1"
    fi


    echo "cloud_${conductor_env}_oct$([ $dc == "all" ] && echo "" || echo "_${dc}")"
}

gen_commands() {
    local perdc="$1"
    local command="$2"

    local envs=${3:-$ENVIRONMENTS}
    local dcs
    local cluster

    for env in $envs; do
        dcs=all
        if [[ $perdc == "perdc" ]] ; then
            dcs=$(get_dc_list $env)
        fi

        for dc in $dcs; do
            cluster=$env
            if [[ $dc != 'all' ]]; then
                cluster="$dc@$env"
            fi

            local conductor_group=$(get_conductor_group $env $dc)

            # CLOUD-70775. GPN certificate should be issued with GPN profile for YC CRT.
            local cluster_command=$command
            if [[ $command == \$YC_ISSUE_CERT_TOOL* ]] && [[ $env == "private-gpn-1" ]]; then
                cluster_command="${cluster_command/\$YC_ISSUE_CERT_TOOL/\$YC_ISSUE_CERT_TOOL --yc-crt-profile gpn}"
            fi
            eval echo $cluster_command
        done
    done
}

echo
echo "# Commands to generate cassandra passwords. Use them for passwords update only"
echo
echo mkdir -p $CASSANDRA_PASSWD_DIR
gen_commands perdc 'tr -cd "[:alnum:]" \< /dev/urandom \| fold -w30 \| head -n1 \> $CASSANDRA_PASSWD_DIR/$cluster.txt'

echo
echo "# Commands to generate rabbitmq passwords. Use them for passwords update only"
echo
echo mkdir -p $RABBIT_PASSWD_DIR
gen_commands perdc 'tr -cd "[:alnum:]" \< /dev/urandom \| fold -w30 \| head -n1 \> $RABBIT_PASSWD_DIR/$cluster.txt'
echo
echo "# Commands to generate rabbitmq cookies. Use them for passwords/cookies update only"
echo
echo mkdir -p $COOKIE_DIR
gen_commands perdc 'tr -cd "[:alnum:]" \< /dev/urandom \| fold -w30 \| head -n1 \> $COOKIE_DIR/$cluster.txt'

echo
echo "# Commands to downloading current cassandra truststores from oct-heads. Use them for cassandra certificates update (see top of the file)"
echo
echo mkdir -p $CASSANDRA_TRUSTSTORE_DIR
gen_commands perdc $'pssh run "\'sudo cp /etc/cassandra/conf/.truststore ~/.truststore; sudo chown $(whoami) ~/.truststore\'" C@${conductor_group}[0]' "$ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE"
gen_commands perdc 'pssh scp C@${conductor_group}[0]:~/.truststore $CASSANDRA_TRUSTSTORE_DIR/$cluster.truststore' "$ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE"
gen_commands perdc $'pssh run "\'rm ~/.truststore\'" C@${conductor_group}[0]' "$ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE"

echo
echo "# Commands to generate oct-head secrets without cassandra passwords"
echo
gen_commands perdc '$YC_ISSUE_CERT_TOOL --skip-secrets oct-database-secrets -o cassandra-existing-truststore=$CASSANDRA_TRUSTSTORE_DIR/$cluster.truststore -- $cluster oct-head' "$ENVIRONMENTS_UPDATE_CURRENT_CASSANDRA_TRUSTSTORE"
gen_commands perdc '$YC_ISSUE_CERT_TOOL --skip-secrets oct-database-secrets -- $cluster oct-head' "$ENVIRONMENTS_GENERATE_NEW_CASSANDRA_TRUSTSTORE"
gen_commands perdc '$YC_ISSUE_CERT_TOOL -- $cluster oct-ipv6' "$ENVIRONMENTS"

echo
echo "# Commands to generate rabbitmq secrets for oct-head. Add '--skip-secrets oct-rabbitmq-cookie oct-rabbitmq-secrets' for updating certificates only, without password and cookies"
echo
gen_commands perdc '$YC_ISSUE_CERT_TOOL -o rabbitmq-cookie-file=$COOKIE_DIR/$cluster.txt,rabbitmq-password-file=$RABBIT_PASSWD_DIR/$cluster.txt -- $cluster oct-rabbitmq'

echo
echo "# Commands to generate oct-client secrets keeping previous passwords/cookies"
echo
gen_commands perstand '$YC_ISSUE_CERT_TOOL $cluster oct-clients'
