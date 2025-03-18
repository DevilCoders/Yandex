#!/bin/bash
set -eu

# runs layer build locally in chroot (in fact, test build_porto_layer.sh script)

. ./common.sh

# create chroot directory
if [ -z "${chroot_dir}" ]; then
	echo "chroot_dir is not set, something went wrong."
    echo "Syntax: $0 <chroot-dir>"
	exit 1
fi
echo "Creating build dir ${chroot_dir}"
mkdir -p ${chroot_dir}
rm -rf ${chroot_dir}/*

# download and unpack ubuntu image
base_system="$root_dir/ubuntu-trusty.tgz"
if [ ! -f ${base_system} ]; then
	wget https://proxy.sandbox.yandex-team.ru/142247241 -O ${base_system}.tmp
    mv "${base_system}.tmp" "${base_system}"
else
    echo "Using existing ${base_system}"
fi

tar xf ${base_system} -C ${chroot_dir}

# set up chroot mounts
setup_chroot

# install environment
cp ${root_dir}/build_porto_layer.sh ${chroot_dir}/tmp
do_chroot /tmp/build_porto_layer.sh
