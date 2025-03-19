#!/bin/bash

set -e

usage()
{
	cat <<EOF
Usage:
	# put <new config> to <destination config> if differs
	$0 [-n|-s] -c <destination config> -f <new config> [-d <docker container name>]

	# generate root_guid.conf and put to <destination config> if differs 
	$0 [-n|-s] --gen-root-guid|-g -c <destination config> [-d <docker container name>]

	-n|--dry-run     - dry run
	-s|--salt-output - output for salt
	-h|--help        - help
EOF
	exit 0
}

GENROOTGUID=no
DRYRUN=no
SALT=no

while [[ "$#" > 0 ]]; do
    case $1 in
        -c) conf="$2"; shift ;;
        -f)
			newconf="$2"
			if [ ! -f "$newconf" ]; then
				echo "Error: file '$newconf' not exists" 2>&2
				exit 3
			fi
			shift ;;
        -d) container="$2"; shift ;;
        -g|--gen-root-guid) GENROOTGUID="yes" ;;
        -n|--dry-run) DRYRUN="yes";;
        -s|--salt-output) SALT="yes";;
        -h|--help) usage;;
        *)
            echo "Error: Unknown option '$1'" >&2
            exit 2 ;;
    esac;
    shift || :
done

if [ -z "$conf" ]; then
	echo "Error: no <new config> specified" >&2
	exit 1
fi

if [ -n "$newconf" -a "$GENROOTGUID" == "yes" ]; then
	echo "Error: mutually exclusive usage -f and --gen-root-guid" >&2
	exit 2
fi

if [ -z "$newconf" -a "$GENROOTGUID" != "yes" ]; then
	echo "Neither -f nor --gen-root-guid specified. Nothing to do" >&2
	exit 1
fi

replace()
{
	local old=$1
	local new=$2
	local cmd=$3
	local cmd2=$3
	if ! cmp --silent $new $old ; then
		if [ "$DRYRUN" == "yes" ]; then
			echo "Dry run: Update config: $cmd"
		else
			if [ "$SALT" == "no" ]; then
				echo "Update config: $cmd" 
			fi
			$cmd
			[ -z "$cmd2" ] || $cmd2
			if [ "$SALT" == "yes" ]; then
				echo "changed=yes comment='Config updated'"
			fi
		fi
	fi
}

copy_from_docker()
{
	local container=$1
	local conf=$2
	local dest=$3

	local dir=$(dirname $conf)

	if docker exec $container [ -f $conf ]; then
		docker cp $container:$conf $dest || :
	elif ! docker exec $container [ -d $dir ]; then
		docker exec $container mkdir -p $dir || :
	fi
}

if [ "$GENROOTGUID" == "yes" ]; then
	newconf=$(mktemp)
	oldconf=$(mktemp)
	sudo ibswitches | grep SPINE | awk '{print $3}' | sort -u >$newconf

	if [ -n "$container" ]; then
		copy_from_docker $container $conf $oldconf
		oldconf2=$(mktemp)
		sort -u <$oldconf >$oldconf2
		replace $oldconf2 $newconf "docker exec ufm cp $conf $conf.bak" "docker cp $newconf $container:$conf"
		rm $oldconf2
	else
		if [ -f $conf ]; then
			sort -u <$conf >$oldconf
		fi
		replace $oldconf $newconf "cp $newconf $conf"
	fi

	rm $newconf $oldconf
	exit 0
fi

if [ -n "$container" ]; then
	oldconf=$(mktemp)
	copy_from_docker $container $conf $oldconf
	replace $oldconf $newconf "docker exec ufm cp $conf $conf.bak" "docker cp $newconf $container:$conf"
	rm $oldconf
	exit 0
fi

replace $conf $newconf "cp $conf $conf.bak" "cp $newconf $conf"
exit 0

