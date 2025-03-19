#!/bin/bash

### BEGIN INIT INFO
# Provides:          pkgver
# Required-Start:    $all
# Required-Stop:     $network $named
# Should-Start:
# Should-Stop:
# Default-Start:     2 3 5
# Default-Stop:      1
# Short-Description: Check packages versions with pkgver
# Description:       Check packages versions with pkgver
### END INIT INFO

# script logic:
#
#     conductor available?
#       /              \
#     yes              no
#       \               |
#        \     wait for conductor to be ready
#         \              /
#         is group *-lost?
#            /        \
#           no        yes
#          /            \
#         /        wait to be
#        /       dropped from *-lost
#       /                  \
#      flag 'pkgver_autosetup'?
#      /               \
#    yes               no
#     |                 |
#  run pkgver           |
#     |                 |
#  run salt             |
#      \            runlevel 2
#       \
#     any fail?
#       / \
#     no   yes
#      \    \
#       \   flag 'pkgver_autosetup_retry'?
#        \                /    \
#         \             yes    no
#          \            /       \
#           \     retry updates  \
#            \                    \
#         runlevel 2            runlevel 1


LOG=/var/log/pkgver/pkgver.log
LOCAL_STATE_FLAGS=/var/lib/pkgver/flags.local
LOCAL_STATE_GROUPS=/var/lib/pkgver/groups.local

HOSTNAME=`hostname -f`

# remove all our warnings from /etc/motd, if they are there
[ -f /etc/motd ] && sed -i /^PKGVER/d /etc/motd

function log {
	echo $(date +"%F %T") $1 >> $LOG
}

function error {
	log "${1}, staying on runlevel 1."
	echo -e "PKGVER: \033[0;31m${1}! Please do something.\033[0m See $LOG for details." >> /etc/motd
	if [ -x /usr/sbin/sendmail ]; then
		sendmail root <<- EOF
		Subject: $HOSTNAME is at runlevel 1
		Machine stopped boot process at runlevel 1.
		See $LOG for details.
		.
EOF
	fi
	exit 1
}


function update_conductor_tags() {
    # Dirty hack to avoid curl bug. On IPv6-only machines first curl run
 	# trying to connect to IPv4 address.
 	curl -s http://c.yandex-team.ru >/dev/null 2>&1

 	while true; do
 		if flags=$(curl -f --retry 5 --retry-delay 2 --connect-timeout 5 -s http://c.yandex-team.ru/api/get_host_tags/$HOSTNAME); then
 			echo $flags > $LOCAL_STATE_FLAGS.tmp && mv $LOCAL_STATE_FLAGS.tmp $LOCAL_STATE_FLAGS
 			break
 		else
 			log "Conductor is not available, cannot determine host tags. Try again in ten seconds."
 			sleep 10
 		fi
 	done
}

function check_tag() {
    tag_name=$1
    if grep -q "$tag_name" $LOCAL_STATE_FLAGS; then
        log "tag ${tag_name} found"
        return 0
    else
        log "tag ${tag_name} NOT found"
        return 1
    fi
}

function autosetup_needed {
	log "Checking if we need to autosetup packages..."
	if check_tag pkgver_autosetup; then
		log "Yes, autosetup is needed."
		return 0
	else
		log "No, autosetup is not needed."
		return 1
	fi
}

function check_packages {
	log "Running pkgver..."
	pkgver.pl >> $LOG 2>&1
	return_value="$?"

	if [ $return_value = 0 ]; then
		log "All packages are up to date."
		return 0
	elif [ $return_value = 1 ]; then
		godmode="--forceyes"

		log "Packages are not up to date, running pkgver in install mode."
		pkgver.pl -i -y $godmode >> $LOG 2>&1
		return_value="$?"
		echo "PKGVER: $return_value" >> $LOG 2>&1
		if [ $return_value != 0 ]; then
			log "Pkgver returned error"
			return 1
		else
			log "Pkgver successfully installed all missing packages."
			return 0
		fi
	else
		error "Pkgver returned error $return_value"
	fi
}

function install_nvidia {
	if lspci | grep NVIDIA | grep V100 > /dev/null; then
		log "NVIDIA V100 Cards found. Checkig for driver"
		if dpkg -l | grep libcuda > /dev/null; then
			log "Driver already installed"
			return 0
		else
			nv_driver_version="$(apt-cache search libcuda1 | awk -F- 'END{ print $2|"sort -uV" }' | sed 's@ @@g')"
			log "Installing NVIDIA driver"
			apt-get install -y --no-install-recommends \
				libcuda1-${nv_driver_version} nvidia-${nv_driver_version}-dev cuda-cudart-10-0 \
				cuda-cudart-10-1 >> $LOG 2>&1 && modprobe nvidia_${nv_driver_version} >> $LOG 2>&1

			return_value="$?"
			echo "NV: $return_value" >> $LOG 2>&1
			if [ $return_value != 0 ]; then
				log "NVIDIA driver installation failed."
				return 1
			else
				log "NVIDIA driver successfully installed."
				return 0
			fi
		fi
	fi
}	

function is_lost {
	# We think, that host require more time to be dropped from lost group.
	# So add retries.

	while true; do
		log "Checking if we are in lost group..."
		if groups=$(curl -f -s --retry 5 --retry-delay 2 --connect-timeout 5 http://c.yandex-team.ru/api/hosts2groups/${HOSTNAME}); then
			echo $groups > $LOCAL_STATE_GROUPS.tmp && mv $LOCAL_STATE_GROUPS.tmp $LOCAL_STATE_GROUPS

			if grep -q "lost$" $LOCAL_STATE_GROUPS; then
				log "We are in lost group. Try again in ten seconds."
				sleep 10
			else
				log "No, we are not lost. :)"
				break
			fi
		else
			log "Conductor is not available. Try again in ten seconds."
			sleep 10
		fi
	done
}

function bug_fix {
	dist_name=$(cat /etc/lsb-release  | grep DISTRIB_CODENAME= | awk -F= '{ print $NF }')

	if [ ! -L /dev/rtc -a "$dist_name" == "xenial" ]; then
	log "Fixing rtc device (Ubuntu bug#???????). Xenial only"
		cd /dev
		ln -sf rtc0 rtc

		return_value="$?"
		echo "RV: $return_value" >> $LOG 2>&1

		if [ $return_value != 0 ]; then
			log "Could not fix /dev/rtc."
			return 1
		else
			log "Rtc bug successfully fixed."
			return 0
		fi
	fi

	log "Hot fix of https://st.yandex-team.ru/STRM-702"
	apt-get install -y s3-mds-metadata-server-ubic >> $LOG 2>&1

	return_value="$?"
	echo "S3: $return_value" >> $LOG 2>&1
	if [ $return_value != 0 ]; then
		log "Could not run s3-mds-metadata-server."
		return 1
	else
		log "Component s3-mds-metadata-server successfully run."
		ubic start s3-mds-metadata-server >> $LOG 2>&1
		return 0
	fi


        return 0
}

function run_salt {
	log "Checking if salt-auto in installed..."
	if dpkg -l yandex-media-common-salt-auto | grep -q "^ii"; then
		if [ -e /usr/bin/salt_update ]; then
			log "/usr/bin/salt_update is present, running it"
			salt_update_opts="--https"
			/usr/bin/salt_update -u $salt_update_opts >> $LOG 2>&1
			return_value="$?"
			echo "SALT_UP: $return_value" >> $LOG 2>&1
			if [ $return_value != 0 ]; then
				log "salt_update failed with code ${return_value}"
				return 1
			fi
			log "Checking for salt-key-cleanup utility"
			if [ -e /usr/bin/salt-key-cleanup ]; then
				log "salt-key-cleanup is present, running it"
				/usr/bin/salt-key-cleanup >> $LOG 2>&1
				return_value="$?"
				echo "SALR_CLEANUP: $return_value" >> $LOG 2>&1
			else
				log "salt-key-clean not found, skipping"
			fi
			log "Running salt-call state.highstate"
			if [ -e /usr/bin/salt-call ]; then
		                salt_opts="--retcode-passthrough"
				/usr/bin/salt-call state.apply queue=True $salt_opts >> $LOG 2>&1
				return_value="$?"
				echo "SALT_APPLY: $return_value" >> $LOG 2>&1
				if [ $return_value != 0 ]; then
					log "salt-call failed with code ${return_value}"
					return 1
				fi
			else
				error "salt-call not found, fail"
			fi
		else
			log "/usr/bin/salt_update not found, skipping"
		fi
	else
		log "salt-auto not found, skipping"
	fi
	return 0
}

function postinst {
	log "Restarting some services"
	service nginx restart && service juggler-client restart

	return_value="$?"
	echo "RESTART: $return_value" >> $LOG 2>&1
	if [ $return_value != 0 ]; then
		log "Could not restart some services."
		return 1
	else
		log "All OK"
		return 0
	fi
}

function start() {
	mv /etc/init.d/pkgver /tmp
	log "Sleeping 60 seconds"
	sleep 60
	for attempt in `seq 1 20`; do
		log "attempt ${attempt} out of 20"
			check_packages && bug_fix && run_salt && postinst && install_nvidia
			return_value="$?"
			echo "FINNISH: $return_value" >> $LOG 2>&1

			if [ $return_value -eq 0 ] ; then
				log "Evertyhing OK. Host is ready"
				return 0
			else
				log "Try again in ten seconds."
				sleep 10
			fi
	done
	error "All attemps failed. Giving up"
}

case "$1" in
	start)
		echo "Running pkgver-hw."
		start
		;;
	stop)
		echo "Stopping pkgver-hw."
		;;
	*)
		echo "Usage: pkgver start"
		exit 1
		;;
esac

