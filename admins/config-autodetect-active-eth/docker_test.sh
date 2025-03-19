#!/bin/bash
run_test() {
    expected="$1"
    # тестируем с debug и без, debug по умолчанию только для eth0
    export debug=$(echo $expected|grep 'eth0$')
    echo true > /usr/local/sbin/autodetect_environment
    unset eth_interfaces
    unset default_route_iface
    unset default_iface
    unset true_eth
    . /build/sbin/autodetect_active_eth
    echo -e "\e[36mexpected_iface=$expected\e[0m"
    echo "eth_interfaces=$eth_interfaces"
    if ! [[ "$expected" =~ [.] ]]; then
        [[ "$eth_interfaces" == "$expected" ]] || exit 1 # assert 
    fi
    echo "default_route_iface=$default_route_iface"
    [[ "$default_route_iface" == "$expected" ]] || exit 1
    echo "default_iface=$default_iface"
    [[ "$default_iface" == "$expected" ]] || exit 1
    expected_true="$expected"
    if [[ "$expected" =~ [.] ]]; then
        expected_true=$(echo $true_eth | sed -r "s/\..*//")
    fi
    echo "true_eth=$true_eth expected $expected_true"
    [[ "$true_eth" == "$expected_true" ]] || exit 1
    echo "all ok for iface=$expected"
}

rename_iface() {
    _from="$1"
    _to="$2"
    if test -z "$_from"; then
        echo "nothing to rename, use iface name $_to"
        return
    fi
    old_default_router=$(ip r|grep -Po 'default via [^\s]+'|awk '{print $NF}')
    echo "rename $_from to $_to"
    ip link set $_from down
    ip link set $_from name $_to
    ip link set $_to up
    echo "restore default router to $old_default_router"
    ip route add default via $old_default_router dev $_to
}

prepare_packages() {
    if test -e /sbin/ip; then
        return
    fi

    if grep Ubuntu /etc/issue.net; then
        apt-get -qq update
        yes | apt-get -y install iproute2 >/dev/null
    else
        if grep 'release 6' /etc/issue.net; then
            # fix obsolete repo
            curl https://www.getpagespeed.com/files/centos6-eol.repo --output /etc/yum.repos.d/CentOS-Base.repo
        fi
        yes | yum -y install iproute >/dev/null
    fi
}

if grep -q docker /proc/1/cgroup ; then
    prepare_packages
    rename_from=""
    for iface in eth0 eth0.123 eno1 enp1s3 enx002590c29f7a; do
        echo -e "\e[35;1m >> iface=$iface\e[0m"
        rename_iface  "$rename_from" "$iface"
        run_test "$iface"
        rename_from="$iface"
    done
else
    set -e
    tempdir="$(mktemp -d)"
    cp -ax . $tempdir/
    pushd $tempdir
    for os in centos:5 centos:6 centos:7 centos:8 ubuntu:lucid ubuntu:precise ubuntu:trusty ubuntu:xenial ubuntu:bionic ubuntu:focal; do
        echo -e "\e[33;1m >> RUN TEST os=$os\e[0m"
        docker run --privileged --rm -it -v $PWD:/build $os bash /build/docker_test.sh
        echo -e "\e[32;1m >> TEST SUCCESS os=$os\e[0m"
    done
    echo -e "\e[32;1m >> ALL TEST SUCCESS"
fi
