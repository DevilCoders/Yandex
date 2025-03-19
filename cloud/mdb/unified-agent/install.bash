#!/bin/bash
set -e

UA_PACKAGE_VERSION="21.11.03"

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y yandex-unified-agent=${UA_PACKAGE_VERSION}
