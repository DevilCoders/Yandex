#!/bin/sh

set -e

CHAIN=COCS_GRACEFUL
DOCKER=0

until [ -z "$1" ] ; do
	case "$1" in
		--with-docker|-d)
			DOCKER=1
		;;
		*)
			echo unknown arg "$1", skipping 1>&2
		;;
	esac
	shift
done

if hostname | grep -q "^front" ; then
	echo "I'm a front"
	echo "Stopping nginx"
	/etc/init.d/nginx stop
	echo "Stopping proxy"
	service cocaine-tornado-proxy stop
	echo "Restarting cocaine-runtime"
	ubic restart cocaine-runtime
	echo "Waiting for 45 seconds"
        sleep 40
        ubic restart cocaine-http-proxy
	sleep 5

	# double check that 'cocaine-tool info' gives an answer
	successes=0
	tries=5

	for i in `seq 1 ${tries}`; do
		set +e
		cocaine-tool i --timeout 10 &> /dev/null
		EXIT_STATUS=$?
		set -e
		sleep 5
		if [ $EXIT_STATUS -eq 0 ]; then
			successes=$((successes+1))
		fi

		if [ $successes -ge 2 ]; then
			break
		fi
	done

	if [ $successes -lt 2 ]; then
		exit 1
	fi
	echo "Got response from cocaine-tool"

	echo "Starting proxy"
	RETRIES=5
	while [ $RETRIES -gt 0 ] ; do
		service cocaine-tornado-proxy start
		sleep 5
		nc -z localhost 10000
		if [ $? -eq 0 ] ; then
			break
		else
			service cocaine-tornado-proxy stop || true
		fi
		(( RETRIES-- ))
	done
	if [ $RETRIES -eq 0 ] ; then
		echo cocaine-tornado-proxy start retries exceeded! nginx will stay OFF! 1>&2
		exit 1
	fi
	echo "Starting nginx"
	/etc/init.d/nginx start
else
	if hostname | grep -q "^cocs" ; then
		echo "I'm a backend"
		echo "Stopping cocaine-runtime"
		ubic stop cocaine-runtime
		if [ "$DOCKER" -eq 1 ] ; then
			echo "Stopping docker"
			service docker stop || true
		fi
		echo "Rejecting port 10053"
		# try to create and then flush it in case chain already exists from previous script run
		iptables -N $CHAIN &>/dev/null || true
		iptables -F $CHAIN &>/dev/null
		ip6tables -N $CHAIN &>/dev/null || true
		ip6tables -F $CHAIN &>/dev/null
		iptables -A $CHAIN ! -i lo -p tcp --dport 10053 -j REJECT --reject-with icmp-admin-prohibited
		iptables -A $CHAIN -j RETURN
		iptables -I INPUT -j $CHAIN
		ip6tables -A $CHAIN ! -i lo -p tcp --dport 10053 -j REJECT --reject-with icmp6-adm-prohibited
		ip6tables -A $CHAIN -j RETURN
		ip6tables -I INPUT -j $CHAIN
		if [ "$DOCKER" -eq 1 ] ; then
			echo "Starting docker"
			service docker start
			echo "Waiting for 3 seconds"
			sleep 3
		fi
		echo "Starting cocaine-runtime"
		ubic start cocaine-runtime
		echo "Waiting for 30 seconds"
		sleep 30

		# double check that 'cocaine-tool info' gives an answer
		successes=0
		tries=20

		for i in `seq 1 ${tries}`; do
			set +e
			cocaine-tool i --timeout 10|python -m json.tool &>/dev/null
			if [ $? = 0 ]; then
				successes=$((successes+1))
			fi
			set -e

			if [ $successes -ge 2 ]; then
				break
			else
				sleep 10
			fi
		done

		if [ $successes -lt 2 ]; then
			echo "cocaine-tool doesn't respond. :("
			echo "Port 10053 is still closed!!!"
			echo "Use this to open:"
			echo "while [ : ] ; do iptables -D INPUT -j $CHAIN &>/dev/null || break ; done"
			echo "iptables -F $CHAIN"
			echo "iptables -X $CHAIN"
			echo "while [ : ] ; do ip6tables -D INPUT -j $CHAIN &>/dev/null || break ; done"
			echo "ip6tables -F $CHAIN"
			echo "ip6tables -X $CHAIN"
			exit 1
		fi
		echo "Got response from cocaine-tool"
		echo "Starting workers"
		set +e
		cocaine-warmup.py
		if [ $? != 0 ]; then
			echo "cocaine-warmup.py failed. :("
			echo "Port 10053 is still closed!!!"
			echo "Use this to open:"
			echo "while [ : ] ; do iptables -D INPUT -j $CHAIN &>/dev/null || break ; done"
			echo "iptables -F $CHAIN"
			echo "iptables -X $CHAIN"
			echo "while [ : ] ; do ip6tables -D INPUT -j $CHAIN &>/dev/null || break ; done"
			echo "ip6tables -F $CHAIN"
			echo "ip6tables -X $CHAIN"
			exit 1
		fi
		set -e
		echo "Opening port 10053"
		set +e
		while [ : ] ; do iptables -D INPUT -j $CHAIN &>/dev/null || break ; done
		set -e
		iptables -F $CHAIN
		iptables -X $CHAIN
		set +e
		while [ : ] ; do ip6tables -D INPUT -j $CHAIN &>/dev/null || break ; done
		set -e
		ip6tables -F $CHAIN
		ip6tables -X $CHAIN
	fi
fi

