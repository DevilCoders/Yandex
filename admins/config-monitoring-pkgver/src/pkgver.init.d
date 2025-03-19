#!/bin/bash

### BEGIN INIT INFO
# Provides:          pkgver
# Required-Start:    $network $named
# Required-Stop:     $network $named
# Should-Start:
# Should-Stop:
# Default-Start:     1
# Default-Stop:
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
	pkgver.pl &>> $LOG
	return_value="$?"

	if [ $return_value = 0 ]; then
		log "All packages are up to date."
		return 0
	elif [ $return_value = 1 ]; then
		godmode=""
		if check_tag pkgver_godmode; then
			log "pkgver godmode active"
			godmode="--forceyes"
		fi

		log "Packages are not up to date, running pkgver in install mode."
		DEBIAN_FRONTEND=noninteractive pkgver.pl -i -y $godmode &>> $LOG
		return_value="$?"
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

function run_salt {
	log "Checking if salt-auto in installed..."
	if dpkg -l yandex-media-common-salt-auto | grep -q "^ii"; then
		if [ -e /usr/bin/salt_update ]; then
			log "/usr/bin/salt_update is present, running it"
			salt_update_opts=""
			check_tag pkgver_autosetup_salt_update_https && salt_update_opts="--https"
			/usr/bin/salt_update -u $salt_update_opts &>> $LOG
			return_value="$?"
			if [ $return_value != 0 ]; then
				log "salt_update failed with code ${return_value}"
				return 1
			fi
			log "Checking for salt-key-cleanup utility"
			if [ -e /usr/bin/salt-key-cleanup ]; then
				log "salt-key-cleanup is present, running it"
				/usr/bin/salt-key-cleanup &>> $LOG
			else
				log "salt-key-clean not found, skipping"
			fi
			log "Running salt-call state.highstate"
			if [ -e /usr/bin/salt-call ]; then
                salt_opts=""
                check_tag pkgver_autosetup_salt_retcode && salt_opts="--retcode-passthrough"
				/usr/bin/salt-call state.highstate queue=True $salt_opts &>> $LOG
				return_value="$?"
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

function start() {
	is_lost
    update_conductor_tags
    if check_tag pkgver_autosetup_retry; then
        attempts="99"
    else
        attempts="1"
    fi

	if autosetup_needed; then
		for attempt in `seq 1 ${attempts}`; do
            log "attempt ${attempt} out of ${attempts}"
			if check_packages && run_salt ; then
				log "Evertyhing OK. Changing to runlevel 2"
				init 2
				return 0
			else
				log "Try again in ten seconds."
				sleep 10
			fi
		done
		error "All attemps failed. Giving up"
	fi
}

case "$1" in
	start)
		echo "Running pkgver."
		start
		;;
	*)
		echo "Usage: pkgver start"
		exit 1
		;;
esac

