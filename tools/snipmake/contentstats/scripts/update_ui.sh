#!/usr/bin/env bash
set -e -o pipefail

mkdir -p ui

cp -rLv $HOME/s/trunk/tools/snipmake/contentstats/assessor_ui ui/
cp -rLv $HOME/s/trunk/tools/snipmake/steam/pyhtml_snapshot ui/
