#!/bin/sh

die () {
    echo "PASSIVE-CHECK:ntp_stratum;$1;$2"
    exit 0
}

#
# check openvz CT via yandex-lib-autodetect-environment package
#
if [ -f /usr/local/sbin/autodetect_environment ] ; then
    . /usr/local/sbin/autodetect_environment
    if [ $is_openvz_host -eq 1 -o $is_lxc_host -eq 1 ] && [ $is_dom0_host -ne 1 ]; then
        die 0 "OK, openvz/lxc CT, skip ntp checking"
    fi
fi

#
# yandex-lib-autodetect-environment is not deployed across FreeBSD hosts and
# knows nothing about jails, so here is check for jail. See also SEEK-6128
#
jailed=`sysctl -n security.jail.jailed 2>/dev/null`
if [ "$jailed" = 1 ]; then
    die 0 "OK, FreeBSD jail, skip ntp checking"
fi

if [ -f /usr/sbin/ntpdate ]; then
    # get current stratum, IP is used instead of "localhost" to avoid IPv4/IPv6 confusion
    # ntpdate is really good in detecting broken NTP servers
    ntp_status=`/usr/sbin/ntpdate -d -t 1 -q 127.0.0.1 2>&1`
    exit_code=$?
    # oops, some `sed` versions have no + in regexp, so the regexp is a bit ugly
    stratum=`echo "$ntp_status" | grep stratum | sed -e 's/.*stratum \([0-9][0-9]*\).*/\1/'`
    status=`echo "$ntp_status" | grep ^127.0.0.1:`

    # 1st assumption: both min and max matter, but only absolute offset values matter
    # 2nd assumption: resolution does not matter, only order of magnitude matters
    ntp_offset=`ntpdc -n -c peers 2>/dev/null | awk '
        function abs(x) {
            if (x<0) {return -1 * x}
            else {return x}
        }
        function fmtsec(x) {
            if (x < 0.1)  { return "< 0.1s" }
            if (x < 1)    { return "< 1s" }
            if (x < 60)   { return sprintf("< %ds", (int(x / 10) + 1) * 10) }
            if (x < 3600) { return sprintf("< %dmin", int(x / 60) + 1) }
                          { return sprintf("< %.1fhour", x / 3600) }
        }

        BEGIN { omin=10e20; omax=0 }
        (NR > 2 && abs($7) < omin) {omin = abs($7)}
        (NR > 2 && omax < abs($7)) {omax = abs($7)}
        END {
            if (omin < 9.9e20 && omax > 0.001) {
                omin=fmtsec(omin);
                omax=fmtsec(omax);
                if (omin != omax) {
                    printf "offset_min %s, offset_max %s", omin, omax
                }
                else {
                    printf "offset %s", omin
                }
            }
        }
    ' 2>&1`

    if [ $(cat /proc/uptime |cut -d'.' -f1) -le 1800 ]; then
	die 0 "uptime less then 30 minutes"
    fi;

    if [ "$exit_code" -ne "0" ]; then
        msg=""
        if [ -n "$ntp_offset" ]; then
            msg="${msg}$ntp_offset, "
        fi
        msg="${msg}stratum is $stratum, status is \"$status\""
        die 2 "Local ntp $msg"
    else
        die 0 "OK"
    fi
elif [ -f /usr/bin/ntpq ]; then
    NORM_PEERS=$(/usr/bin/ntpq -nc opeers 2>/dev/null | awk -v c=0 '$1 !~ /(remote|=+)/ && $3 < 16 {c++}END{print c}')
    if [ $NORM_PEERS -eq 0 ]; then
        die 2 "no valid ntp peers found"
    else
        die 0 "OK"
    fi
else
    chrony_info=$(chronyc tracking 2>&1)
    exit_code=$?
    status=$(chronyc tracking 2>&1 | grep status | awk '{print $NF}')
    offset=$(chronyc tracking 2>&1 | grep "Last offset" | awk '{print $(NF-1)}')
    status=$(chronyc tracking 2>&1 | grep Stratum | awk '{print $NF}')
    if [ "$exit_code" -ne "0" ]; then
        msg=""
        if [ -n "$ntp_offset" ]; then
            msg="${msg}$ntp_offset, "
        fi
        msg="${msg}stratum is $stratum, status is \"$status\""
        die 2 "Local ntp $msg"
    else
        die 0 "OK, offset: $offset"
    fi
fi
