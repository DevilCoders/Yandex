#!/bin/bash

set -ex

export KMS_HOST_NAME=$1

`dirname $0`/../scripts/do_install_packages.py
