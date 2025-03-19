#!/bin/sh

debmirror \
	--no-check-gpg \
	--ignore-release-gpg \
	--i18n \
	--host old-releases.ubuntu.com \
	--method=http \
	--root=ubuntu \
	--dist=precise,precise-updates,precise-backports,precise-proposed,precise-security \
	--arch=amd64 \
	--progress \
	--nosource \
	--postcleanup \
	/mirror/old-ubuntu/
