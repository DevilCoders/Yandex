#!/bin/bash
set -eu
. ./common.sh

# set up chroot mounts
setup_chroot

# run a shell
do_chroot /bin/bash
