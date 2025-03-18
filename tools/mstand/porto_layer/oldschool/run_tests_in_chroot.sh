#!/bin/bash
set -eu

# runs mstand test 
. ./common.sh

setup_chroot

echo "Copying mstand sources"

# TODO: fix this bullshit with rsync:
# 1. run mstand package creation
# 2. unpack it in chroot


# cd "${root_dir}/package"
# sh build-mstand-package.sh

rsync -rv $(dirname ${root_dir}) ${chroot_dir}/tmp --exclude="nirvana_env/*" --exclude="work_dir/*" --exclude="*.pyc" --exclude="tests/integration/run_dir" --links
do_chroot /bin/sh -x -c '. /root/.bashrc.local; cd /tmp/mstand; python /usr/local/bin/py.test; python /usr/local/bin/py.test -c pytest_fat.ini' || true
