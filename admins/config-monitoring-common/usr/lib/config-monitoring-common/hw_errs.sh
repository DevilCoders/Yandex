#!/bin/sh
#
# Tag kernel reported errors (from dmesg)
#
# $Id: hw_errs.sh,v 1.40 2007/11/16 10:42:41 andozer Exp $
#
me=${0##*/}     # strip path
me=${me%.*}     # strip extension
BASE=$HOME/agents
CONF=/etc/monitoring/$me.conf
TMP=/tmp
PREV=$TMP/$me.prev
WATCH_LAST=3600 # seconds, e.g. 3 hours
PATH=/bin:/sbin:/usr/bin:/usr/sbin
TSTAMP=`date '+%s'`
ALARM_PAT='error|warning|fail|\(da[0-9]+:[a-z0-9]+:[0-9]+:[0-9]+:[0-9]+\)|Non-Fatal\ Error\ DRAM\ Controler|nf_conntrack|task\ abort'
IGNORE_PAT='acpi_throttle|ehci_hcd|uses\ 32-bit\ capabilities|GHES:\ Poll\ interval|acpi_throttle[0-9]:\ failed\ to\ attach\ P_CNT|aer|aer_init:\ AER\ service\ init\ 
fails|aer:\ probe\ of|arplookup|(at [0-9a-f]+)? rip[: ][0-9a-f]+ rsp[: ][0-9a-f]+ error[: ]|Attempt\ to\ query\ 
device\ size\ failed|check_tsc_sync_source\ failed|failed\ SYNCOOKIE\ authentication|igb:|ipfw:\ pullup\ 
failed|Marking\ TSC\|mfi|MOD_LOAD|MSI\ interrupts|nfs\ send\ error\ 32\ |NO_REBOOT|optimal|page\ allocation\ 
failure|PCI\ error\ interrupt|pid\ [0-9]+|rebuild|Resume\ from\ disk\ 
failed|rwsem_down|smb|swap_pager_getswapspace|thr_sleep|uhub[0-9]|ukbd|usbd|EDID|Error\ Record\ Serialization\ 
Table.+initialized|end_device-[0-9]+:[0-9]+:\ dev\ error\ handler|nf_conntrack\ version'
CRIT_PAT='I/O|medium|defect|mechanical|retrying|broken|degraded|offline|failed|unconfigured_bad|conntrack:\ table\ full|xfs_log_force'


[ -s $CONF ] && . $CONF

[ -d $TMP ] || mkdir -p $TMP
[ -s $TMP/$me.dmesg.prev ] || touch $TMP/$me.dmesg.prev
[ -s $TMP/$me.msg.prev ] || touch $TMP/$me.msg.prev

die () {
        c=$1
        [ $c -eq 2 -a ${NOCRIT:-0} -eq 1 ] && c=1 
        echo "PASSIVE-CHECK:$me;$c;$2"
        exit 0
}

#
# check openvz CT via yandex-lib-autodetect-environment package
#
if [ -f /usr/local/sbin/autodetect_environment ] ; then
        . /usr/local/sbin/autodetect_environment
        if [ $is_virtual_host -eq 1 ] ; then
                die 0 "OK, openvz CT, skip hw_errs checking"
        fi
fi

# Check if service is disabled. Details in ticket 2010070611000124
[ "${DISABLED:-0}" -eq 1 ] && die 0 OK

dmesg | tail -n +2 | grep -i -E "$ALARM_PAT" | grep -v -E "$IGNORE_PAT" > $TMP/$me.dmesg.cur
[ -e $TMP/$me.dmesg.cur ] || touch $TMP/$me.dmesg.cur

diff $TMP/$me.dmesg.prev $TMP/$me.dmesg.cur | sed -e '/> / {
        s///
        p
}
d
' | awk -v t=$TSTAMP '{print t, $0}' >> $TMP/$me.msg.prev

awk -v p=$(($TSTAMP-$WATCH_LAST)) '$1>=p {print}' $TMP/$me.msg.prev > $TMP/$me.msg.cur
mv $TMP/$me.dmesg.cur $TMP/$me.dmesg.prev
mv $TMP/$me.msg.cur $TMP/$me.msg.prev
[ -s $TMP/$me.msg.prev ] || die 0 OK

c_err=`grep -i -E "$CRIT_PAT" $TMP/$me.msg.prev | tail -n 1 | sed -e 's/^[0-9]* //'`
[ -n "$c_err" ] && die 2 "$c_err"
err=`tail -n 1 $TMP/$me.msg.prev | sed -e 's/^[0-9]* //'`
die 1 "$err"
