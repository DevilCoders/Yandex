#@IgnoreInspection BashAddShebang
# "strict mode"
# crash on error
set -e
# crash on undefined variable
set -u

if [ "`id -u`" -ne 0 ]; then
	echo "This script needs to be run as root!"
	exit 1
fi

root_dir=`dirname $(readlink -f $0)`
chroot_dir="$root_dir/ubuntu"

teardown_chroot() {
	set -e
	# clean up as much as we can
	umount ${chroot_dir}/etc/resolv.conf
    umount ${chroot_dir}/dev
	rm -rf ${chroot_dir}/tmp
	set +e
}

setup_chroot() {
	if [ ! -f "${chroot_dir}/bin/bash" ]; then
		echo "chroot doesn't have bash, maybe build it first?"
		exit 1
	fi

	trap 'teardown_chroot' EXIT

	# needed for pip to work because magic
	mount --bind /dev ${chroot_dir}/dev
	mount --bind /etc/resolv.conf ${chroot_dir}/etc/resolv.conf
}

do_chroot() {
	(
		# work around path issues on Arch
		export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
        echo "Executing in chroot: $@"
		chroot ${chroot_dir} "$@"
	)
}
