#!/usr/bin/env bash

# this script updates all hardware and clean up unworking machines
# you should run it after first recluster stage if you want check hw in production groups

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

# update hw params of all hosts
# USE BEFORE sync_with_bot !!!!!!!!!!!!!!!!!!!
function update_hw() {
    if [ -z $1 ]; then
        groups="MSK_RESERVED,SAS_RESERVED,MAN_RESERVED"
    else
        groups=$1
    fi

    run utils/pregen/update_hosts.py -a update -g ${groups} --ignore-unknown-lastupdate --update-older-than 5 --ignore-detect-fail -y
}

# update host list from bot
function sync_with_bot() {
    echo "Get extra bot machines to prenalivka"
    run ./utils/pregen/get_extra_bot_machines.py -a addnew -y

    echo "Update prenalivka and distribute to reserved"
    run ./utils/pregen/update_prenalivka.py -g ALL_PRENALIVKA --use-heartbeat-db -y

    echo "Update ALL_UNWORKING_NO_MTN"
    GOOD_HOSTS=`./utils/common/dump_hostsdata.py -i name -g ALL_UNWORKING_NO_MTN -f "lambda x: 'vlan688' in x.vlans and 'vlan788' in x.vlans" | xargs | sed 's/ /,/g'`
    if [ -n "${GOOD_HOSTS}" ]; then
        ./utils/common/update_igroups.py -a movetoreserved -s "${GOOD_HOSTS}"
    fi

    # echo "Remove unexistings hosts from ALL_PRENALIVKA/RESERVE_TRANSFER/ALL_UNSORTED*"
    run ./utils/pregen/get_extra_bot_machines.py -a removeold -f "lambda x: x.card.name in ['ALL_PRENALIVKA', 'RESERVE_TRANSFER'] or x.card.tags.metaprj == 'unsorted'" -y

    echo "Updated unsorted groups"
    run ./utils/common/categorize_unsorted.py -l 100 -y

    echo "Update machines with unknown cpu model"
    run ./utils/pregen/get_extra_bot_machines.py -a update_hosts_info -y
}

# rename all hosts. Pretty dangerous due to outdated inventory number information
function rename_by_invnum() {
    run utils/pregen/rename_hosts.py -m oops -s ALL -f "lambda x: x.is_vm_guest() is False" -i
}

function update_tiers_sizes() {
    if [ -z $1 ]; then
        intlookups="ALL"
    else
        intlookups=$1
    fi

    run tools/analyzer/analyzer.py -f test.sqlite -u instance_database_size -i ${intlookups} -t 200 -q
    run utils/pregen/update_tiers_sizes.py -d test.sqlite
}

function update_sas_optimizer_weights() {
    if [ -z $1 ]; then
        echo "Function update_sas_weights accepts at least one argument"
        exit 1
    fi

    run ./tools/analyzer/analyzer.py -f weights.sqlite -s `echo "$@" | xargs echo | sed 's/ /,/g'` -t 400 -u instance_cpu_usage,host_cpu_usage -q
    for config in "$@"; do
        run ./utils/common/update_sas_optimizer_config.py -c ${config} -d weights.sqlite --use-all-instances-stats -y
    done
}

# function to move unworking machines from specified groups to corresponding unworking ones
function cycle_hosts() {
    if [ -z $1 ]; then
        echo "Function cycle_hosts accepts at least one argument (comma-separated group list)"
        exit 1
    fi

    run utils/pregen/check_alive_machines.py -c sshport -g $1 -u ALL_UNWORKING -n -v
}

# find unworking machines and move them to corresponding group, move repaired machines to reserved
function update_unworking() {
    run utils/pregen/check_alive_machines.py -c sshport -g ALL_UNWORKING -r -n -v

    cycle_hosts MSK_RESERVED,SAS_RESERVED,MAN_RESERVED

    run utils/sky/show_hosts_with_raised_instances.py -s ALL_UNWORKING_BUSY -p skynet -a show_busy_detailed --move-to-reserved
    run utils/sky/show_hosts_with_raised_instances.py -s ALL_RUSTY_BUSY -p skynet -a show_busy_detailed --move-to-group ALL_RUSTY
}

# find unworking machines in production groups and move them to unworking
function update_unworking_production() {
    if [ -z $1 ]; then
        groups="MAN_WEB_BASE,SAS_WEB_BASE,MSK_WEB_BASE,MSK_RESERVED,SAS_RESERVED,MAN_RESERVED"
    else
        groups=$1
    fi

    cycle_hosts ${groups}
}

function move_trunk_reserved() {
    if [ $# -ne 1 ]; then
        echo "You should pass exactly one argument to <move_trunk_reserved> func"
        exit 1
    fi

    CLEAN_OLD=$1
    if [ ${CLEAN_OLD} -eq 1 ]; then
        echo "Moving *_RESERVED_OLD -> *_RESERVED"
        P1="_OLD"
        P2=""
    else
        echo "Moving *_RESERVED -> *_RESERVED_OLD"
        P1=""
        P2="_OLD"
    fi

    for group in MSK_RESERVED SAS_RESERVED MAN_RESERVED; do
        run ./utils/common/update_igroups.py -a emptygroup -g ${group}${P1} -c ${group}${P2}
    done
}
