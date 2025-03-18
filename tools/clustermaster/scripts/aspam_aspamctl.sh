#!/bin/sh

./aspam_walrus_clusterctl.sh \
	-a /Berkanavt/antispam/cm/aspam/auth.key \
	-v /Berkanavt/antispam/cm/aspam/var \
	-p /Berkanavt/antispam/cm/aspam/master.pid \
	-P /Berkanavt/antispam/cm/aspam/worker.pid \
	-s /Berkanavt/bin/scripts/spm_lib \
	-c /Berkanavt/database/host.cfg \
	-u /aspam/run \
	-U /aspam/ro \
	-R 3129 \
	-e "wspm wspm2 ya" \
	-E "wspm2 ya walrus primus" \
	"$@"
