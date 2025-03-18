#!/bin/bash -x

set -e

# Defaults:
# "don't bother me with editing".
if [ -z "$EDITOR" ]; then export EDITOR="true"; fi
# Increment the second number.
if [ -z "$SBDNR_VERSION_PART"]; then SBDNR_VERSION_PART="1"; fi

# Enforced:
SBDNR_TAGPREFIX=" "
SBDNR_RELEASE_TO="yandex-precise"
SBDNR_PYPI_RELEASE_TO="yandex"

. $(which sbdutils_newrelease.sh)
