#!/bin/bash

set -x

clean_up() {
    [ "${MNT_DIR}" != "" ] && lsof -t "${MNT_DIR}" | xargs -r kill -9 && sleep 1s
    [ "${MNT_DIR}" != "" ] && umount "${MNT_DIR}/proc/" && sleep 1s
    [ "${MNT_DIR}" != "" ] && umount "${MNT_DIR}/sys/" && sleep 1s
    [ "${MNT_DIR}" != "" ] && umount "${MNT_DIR}/dev/" && sleep 1s
    [ "${MNT_DIR}" != "" ] && umount "${MNT_DIR}" && sleep 1s
    [ "${DISK}" != "" ] && qemu-nbd -d "${DISK}" && sleep 1s
    [ "${MNT_DIR}" != "" ] && rm -r "${MNT_DIR}" && sleep 1s
}

fail() {
    clean_up
    echo ""
    echo "FAILED: $1"
    exit 1
}

cancel() {
    fail "CTRL-C detected"
}

if [ $# -lt 1 ]
then
    echo "usage: $0 <image-file> [optional debootstrap args]" 1>&2
    exit 1
fi

FILE=$1
shift 1

trap cancel INT

echo "Installing into $FILE..."

MNT_DIR=$(mktemp -d)
rm -rf "${MNT_DIR}"
mkdir "${MNT_DIR}"
DISK=

echo "Looking for nbd device..."

modprobe nbd max_part=16 || fail "failed to load nbd module into kernel"

qemu-img create -f qcow2 -o cluster_size=1M "${FILE}" 20G || fail "failed to create image ${FILE}"

for i in /dev/nbd*
do
    if qemu-nbd -c "${i}" "${FILE}"
    then
        DISK="${i}"
        break
    fi
done

[ "${DISK}" == "" ] && fail "no nbd device available"

echo "Connected ${FILE} to ${DISK}"

echo "Partitioning ${DISK}..."
echo '2048;' | sfdisk "${DISK}" -q || fail "Unable to partition ${FILE}"

echo "Creating FS on root partition..."
mkfs.ext4 -q "${DISK}p1" || fail "cannot create / ext4"

echo "Mounting root partition..."
mount "${DISK}p1" "${MNT_DIR}" || fail "cannot mount /"

echo "Installing Ubuntu..."
debootstrap \
    --exclude=ubuntu-minimal,resolvconf \
    --arch amd64 \
    "$@" \
    bionic \
    "${MNT_DIR}" \
    http://mirror.yandex.ru/ubuntu || fail "Unable to install into ${DISK}"

echo "Configuring system..."
cat <<EOF > "${MNT_DIR}/etc/fstab"
$(blkid ${DISK}p1 -s UUID -o export) /                   ext4    errors=remount-ro 0       1
EOF

mkdir -p "${MNT_DIR}/boot/grub"

echo vm-image-template.db.yandex.net > "${MNT_DIR}/etc/hostname"

cat <<EOF > "${MNT_DIR}/etc/hosts"
127.0.0.1       localhost
fd01:ffff:ffff:ffff::2 vm-image-template.db.yandex.net

::1     localhost ip6-localhost ip6-loopback
ff02::1 ip6-allnodes
ff02::2 ip6-allrouters
EOF

cat <<EOF > "${MNT_DIR}/etc/network/interfaces"
auto lo
iface lo inet loopback

# Static configuration is required for later setup
auto eth0
iface eth0 inet6 static
         address fd01:ffff:ffff:ffff::2
         netmask 96
         dad-attempts 0
         network fd01:ffff:ffff:ffff::
         post-up /sbin/ip -6 route replace default via fd01:ffff:ffff:ffff::1 mtu 1450 advmss 1390
EOF

mount --bind /dev/ "${MNT_DIR}/dev" || fail "cannot bind /dev"
chroot "${MNT_DIR}" mount -t proc none /proc || fail "cannot mount /proc"
chroot "${MNT_DIR}" mount -t sysfs none /sys || fail "cannot mount /sys"
cp bionic-sources.list "${MNT_DIR}/etc/apt/sources.list"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -qq update || fail "Unable to run apt-get update"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -y upgrade || fail "Unable to run apt-get upgrade"
cp mdb-bionic-secure-stable.list "${MNT_DIR}/etc/apt/sources.list.d/"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" locale-gen en_US.UTF-8 || fail "Unable to gen locale"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -qq -o Acquire::AllowInsecureRepositories=true \
    -o Acquire::AllowDowngradeToInsecureRepositories=true update || fail "Unable to run apt-get update (2nd time)"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -o APT::Get::AllowUnauthenticated=true \
    -y install yandex-archive-keyring || fail "Unable to install yandex-archive-keyring"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -qq update || fail "Unable to run apt-get update (3rd time)"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get install -y grub-pc || fail "cannot install grub"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get install -y \
    linux-image-generic=4.19.143-37 \
    linux-tools=4.19.143-37 || fail "cannot install linux-image and linux-tools"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -y install virt-what tcpdump || fail "cannot install virt-what and tcpdump"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -y install \
    salt-common=3000.9+ds-1+yandex0 \
    salt-minion=3000.9+ds-1+yandex0 \
    python-msgpack=3-4fc9b70 \
    mdb-config-salt=1.8245437-trunk \
    yandex-search-user-monitor=1.0-45 \
    python-boto \
    python-botocore \
    python-concurrent.futures \
    mdb-ssh-keys \
    parted \
    yandex-default-locale-en || fail "Unable to install salt"

mkdir -p "${MNT_DIR}/etc/cloud/cloud.cfg.d"

mkdir -p "$MNT_DIR/etc/salt/pki/minion"
cp -f /etc/dbaas-vm-setup/minion.{pub,pem} "$MNT_DIR/etc/salt/pki/minion/"
cp -f /etc/dbaas-vm-setup/master_sign.pub "$MNT_DIR/etc/salt/pki/minion/"
mkdir -p "$MNT_DIR/etc/yandex/mdb-deploy"
echo 2 > "$MNT_DIR/etc/yandex/mdb-deploy/deploy_version"
echo deploy-api.db.yandex-team.ru > "$MNT_DIR/etc/yandex/mdb-deploy/mdb_deploy_api_host"
LANG=C chroot "${MNT_DIR}" grub-install "${DISK}" || fail "cannot install grub"
cp grub.default "${MNT_DIR}/etc/default/grub"
cp ttyS0.conf "${MNT_DIR}/etc/init/ttyS0.conf"
LANG=C chroot "${MNT_DIR}" update-grub || fail "cannot update grub"
LANG=C chroot "${MNT_DIR}" systemctl disable \
    systemd-networkd.socket \
    systemd-networkd \
    systemd-timesyncd \
    systemd-resolved \
    networkd-dispatcher \
    systemd-networkd-wait-online || fail "cannot disable systemd services"
LANG=C chroot "${MNT_DIR}" systemctl mask \
    systemd-networkd.socket \
    systemd-networkd \
    systemd-timesyncd \
    systemd-resolved \
    networkd-dispatcher \
    systemd-networkd-wait-online || fail "cannot mask systemd services"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -y \
    install ifupdown || fail "cannot install ifupdown"
LANG=C DEBIAN_FRONTEND=noninteractive chroot "${MNT_DIR}" apt-get -y \
    purge nplan netplan.io || fail "cannot remove netplan"
rm -f "${MNT_DIR}/etc/salt/minion_id"
rm -f "${MNT_DIR}/etc/dhcp/dhclient-enter-hooks.d/resolved"

rm -f "${MNT_DIR}/etc/resolv.conf"
cat <<EOF > "${MNT_DIR}/etc/resolv.conf"
nameserver 2a02:6b8::1:1
nameserver 2a02:6b8:0:3400::1
options timeout:1 attempts:1
EOF

sed -i "s|${DISK}p1|/dev/vda1|g" "${MNT_DIR}/boot/grub/grub.cfg"

echo "Finishing grub installation..."
grub-install "${DISK}" --root-directory="${MNT_DIR}" --modules="biosdisk part_msdos" || fail "cannot reinstall grub"

KEY_NAME=$(echo ${FILE} | sed 's/\.img//g')
ssh-keygen -P '' -t ecdsa -f "${KEY_NAME}" -C 'TEMPKEY'
cat "${KEY_NAME}".pub > "${MNT_DIR}/root/.ssh/authorized_keys"

echo "SUCCESS!"
clean_up
exit 0
