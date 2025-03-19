#!/bin/sh

die () {
        echo "$1;$2"
        exit 0
}

if [ -f /usr/local/sbin/autodetect_environment ] ; then
    is_virtual_host=0
    . /usr/local/sbin/autodetect_environment >/dev/null 2>&1 || true
    if [ "$is_virtual_host" -eq 1 -a "$is_kvm_host" -eq 0 ] ; then
        die 0 "OK"
    fi
fi

# get current stratum, IP is used instead of "localhost" to avoid IPv4/IPv6 confusion
# ntpdate is really good in detecting broken NTP servers
ntp_status=$(/usr/sbin/ntpdate -d -t 1 -q 127.0.0.1 2>&1)
exit_code=$?
# oops, some `sed` versions have no + in regexp, so the regexp is a bit ugly
stratum=$(echo "$ntp_status" | grep stratum | sed -e 's/.*stratum \([0-9][0-9]*\).*/\1/')
status=$(echo "$ntp_status" | grep ^127.0.0.1:)

# 1st assumption: both min and max matter, but only absolute offset values matter
# 2nd assumption: resolution does not matter, only order of magnitude matters
ntp_offset=$(/usr/sbin/ntpdc -n -c peers 2>/dev/null | awk '
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
    ' 2>&1)

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
