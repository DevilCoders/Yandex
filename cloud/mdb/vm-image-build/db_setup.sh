#!/bin/bash

set -x

clean_up() {
    [ "${KVM_PROC}" != "" ] && kill -0 "${KVM_PROC}" && kill -9 "${KVM_PROC}"
    [ "${MNT_DIR}" != "" ] && lsof -t "${MNT_DIR}" | xargs -r kill -9 && sleep 1s
    [ "${MNT_DIR}" != "" ] && umount "${MNT_DIR}" && sleep 1s
    [ "${DISK}" != "" ] && qemu-nbd -d "${DISK}" && sleep 1s
    [ "${MNT_DIR}" != "" ] && rm -r "${MNT_DIR}" && sleep 1s
    [ "${VTYPE}" == "porto" ] && [ "${DATABASE}" != "" ] && rm -f "${DATABASE}.img" && sleep 1s
    [ "${QEMU_NBD_PROC}" != "" ] && kill -0 "${QEMU_NBD_PROC}" && kill -9 "${QEMU_NBD_PROC}"
}

fail() {
    clean_up
    echo ""
    echo "FAILED $1"
    exit 1
}

cancel() {
    fail "CTRL-C detected"
}

ssh_cmd() {
    ssh root@fd01:ffff:ffff:ffff::2 -i "${KEY_NAME}" "$@"
}

if [ $# -lt 4 ]
then
    echo "usage: $0 <image name> <suite name> <source image> <vtype>" 1>&2
    exit 1
fi

DATABASE="$1"
SRC_IMAGE="$3"
VTYPE="$4"
GRAINS="\"grains: {test: {suite: $2, vtype: ${VTYPE}}}\""
KEY_NAME=$(echo ${SRC_IMAGE} | sed 's/\.img//g')

trap cancel INT

cp "${SRC_IMAGE}" "${DATABASE}.img"

kvm -drive file="${DATABASE}.img",if=virtio \
    -m 2048 \
    -cpu host \
    -nographic \
    -net nic \
    -net tap,script=./tap_up.sh >"${DATABASE}_vm.log" 2>&1 &

KVM_PROC=$!

for i in $(seq 60)
do
    timeout 1 nc -z fd01:ffff:ffff:ffff::2 22 && CONNECT_OK=1 && break
    sleep 1s
done

[ "${CONNECT_OK}" == "1" ] || fail "Unable to connect to vm"

ssh_cmd "DEBIAN_FRONTEND=noninteractive apt-get -y --force-yes install mdadm" || fail "unable to install mdadm"
ssh_cmd "cp /etc/salt/minion /etc/salt/minion.bak" || fail "unable to backup minion config"
ssh_cmd "echo $GRAINS > /etc/salt/minion" || fail "unable to set suite info in minion config"
# Workaround for mongodb hosts (first highstate always fails)
ssh_cmd "salt-call saltutil.sync_all; salt-call state.highstate; salt-call state.highstate" | tee "${DATABASE}_salt.log" | grep Failed: | tail -n1 | grep -qE 'Failed:\s+0' || fail "unable to run highstate"
ssh_cmd "update-rc.d -f salt-minion remove" || fail "unable to disable autostart of salt-minion"
ssh_cmd "mv -f /etc/salt/minion.bak /etc/salt/minion" || fail "unable to restore minion config"
if [ "${VTYPE}" == "porto" ]
then
    # TODO install yandex-selfdns-client from salt
    ssh_cmd "DEBIAN_FRONTEND=noninteractive apt-get -y --force-yes install yandex-selfdns-client" || fail "unable to install yandex-selfdns-client"
fi
ssh_cmd "apt-get clean" || fail "unable to clear apt cache"
ssh_cmd "poweroff"
sleep 180s

for i in $(seq 60)
do
    if kill -0 "${KVM_PROC}"
    then
        kill -9 "${KVM_PROC}"
    else
        break
    fi
    sleep 1s
done

MNT_DIR=$(mktemp -d)
rm -rf "${MNT_DIR}"
mkdir "${MNT_DIR}"
DISK=

echo "Looking for nbd device..."

for i in /dev/nbd*
do
    if qemu-nbd -c "${i}" "${DATABASE}.img"
    then
        DISK="${i}"
        QEMU_NBD_PROC = $!
        break
    fi
done

[ "${DISK}" == "" ] && fail "no nbd device available"

echo "Connected ${DATABASE}.img to ${DISK}"

partprobe "${DISK}"

echo "Mounting root partition..."
mount "${DISK}p1" "${MNT_DIR}" || fail "cannot mount /"

truncate -s 0 "${MNT_DIR}/etc/hostname"

cat <<EOF > "${MNT_DIR}/etc/hosts"
127.0.0.1       localhost

::1     localhost ip6-localhost ip6-loopback
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
EOF

grep -v TEMPKEY "${MNT_DIR}/root/.ssh/authorized_keys" > "${MNT_DIR}/root/.ssh/authorized_keys.new"
mv "${MNT_DIR}/root/.ssh/authorized_keys.new" "${MNT_DIR}/root/.ssh/authorized_keys"

rm -rf \
    "${MNT_DIR}"/var/lib/postgresql/{1,2}* \
    "${MNT_DIR}"/var/lib/postgresql/.pgpass \
    "${MNT_DIR}"/home/mysql/.my.cnf \
    "${MNT_DIR}"/home/mysql/.restore.my.cnf \
    "${MNT_DIR}"/etc/postgresql/{1,2}* \
    "${MNT_DIR}"/etc/postgresql/ssl/* \
    "${MNT_DIR}"/etc/pgbouncer/ssl/* \
    "${MNT_DIR}"/etc/pgbouncer/userlist.txt \
    "${MNT_DIR}"/root/.pgpass \
    "${MNT_DIR}"/root/.my.cnf \
    "${MNT_DIR}"/var/lib/push-client* \
    "${MNT_DIR}"/usr/local/yasmagent/CONF/agent.mailpostgresql.conf \
    "${MNT_DIR}"/var/lib/clickhouse/* \
    "${MNT_DIR}"/etc/clickhouse-server/* \
    "${MNT_DIR}"/etc/cron.d/clickhouse-server* \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/clickhouse-server.service \
    "${MNT_DIR}"/lib/systemd/system/clickhouse-server.service \
    "${MNT_DIR}"/var/lib/mongodb/* \
    "${MNT_DIR}"/etc/mongodb/* \
    "${MNT_DIR}"/lib/systemd/system/mongodb.service \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/mongodb.service \
    "${MNT_DIR}"/var/lib/zookeeper/* \
    "${MNT_DIR}"/etc/zookeeper/* \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/zookeeper.service \
    "${MNT_DIR}"/lib/systemd/system/zookeeper.service \
    "${MNT_DIR}"/var/lib/elasticsearch/* \
    "${MNT_DIR}"/etc/elasticsearch/* \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/elasticsearch.service \
    "${MNT_DIR}"/lib/systemd/system/elasticsearch.service \
    "${MNT_DIR}"/var/lib/opensearch/* \
    "${MNT_DIR}"/etc/opensearch/* \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/opensearch.service \
    "${MNT_DIR}"/lib/systemd/system/opensearch.service \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/kibana.service \
    "${MNT_DIR}"/lib/systemd/system/kibana.service \
    "${MNT_DIR}"/etc/kibana/* \
    "${MNT_DIR}"/home/monitor/.pgpass \
    "${MNT_DIR}"/home/monitor/.my.cnf \
    "${MNT_DIR}"/var/cache/salt/minion/* \
    "${MNT_DIR}"/etc/salt/minion_id \
    "${MNT_DIR}"/etc/salt/pki/minion/minion.p* \
    "${MNT_DIR}"/etc/salt/pki/minion/master_sign.pub \
    "${MNT_DIR}"/etc/yandex/mdb-deploy/deploy_version \
    "${MNT_DIR}"/etc/yandex/mdb-deploy/mdb_deploy_api_host \
    "${MNT_DIR}"/var/cache/apt/archives/*.deb \
    "${MNT_DIR}"/var/log/journal/* \
    "${MNT_DIR}"/var/lib/mysql/* \
    "${MNT_DIR}"/etc/mysql/* \
    "${MNT_DIR}"/etc/rc*/*mysql \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/mysql.service \
    "${MNT_DIR}"/var/lib/redis/* \
    "${MNT_DIR}"/etc/redis/* \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/redis-sentinel.service \
    "${MNT_DIR}"/lib/systemd/system/redis-sentinel.service \
    "${MNT_DIR}"/etc/systemd/system/multi-user.target.wants/redis-server.service \
    "${MNT_DIR}"/lib/systemd/system/redis-server.service \
    "${MNT_DIR}"/home/gpadmin/{.pgpass,.ssh,gpAdminLogs,gpconfigs} \
    "${MNT_DIR}"/var/lib/greenplum/* \
    "${MNT_DIR}"/etc/greenplum-pxf/*


find "${MNT_DIR}"/var/log -name '*.gz' -delete
find "${MNT_DIR}"/var/log -regextype posix-egrep -regex '.*[0-9]$' -delete
find "${MNT_DIR}"/var/log -type f -exec truncate -s 0 {} \;

if [ "${VTYPE}" == "porto" ]
then
    # TODO manage /etc/network/interfaces from salt
    echo "source-directory /etc/network/interfaces.d/*" > "${MNT_DIR}"/etc/network/interfaces
    tar -cvpf "${DATABASE}_porto.img" -C "${MNT_DIR}" .
    gzip -f "${DATABASE}_porto.img"
fi

clean_up
exit 0
