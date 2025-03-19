#!/bin/sh

if [ `whoami` != 'root' ] ; then
        echo "this script runs only under root account"
        exit 1
fi

if [ /sbin/modinfo cpufreq_powersave 2>/dev/null ]; then
	/sbin/modprobe cpufreq_powersave
fi

if [ /sbin/modinfo cpufreq_ondemand 2>/dev/null ]; then
	/sbin/modprobe cpufreq_ondemand
fi

if [ /sbin/modinfo acpi-cpufreq 2>/dev/null ]; then
	/sbin/modprobe acpi-cpufreq
fi

if [ ! -d /sys/devices/system/cpu/cpu0/cpufreq ] ; then
	exit 0
fi

MAXCPU=$[`fgrep -c processor /proc/cpuinfo` - 1]
for i in `seq 0 $MAXCPU` ; do
	echo performance > "/sys/devices/system/cpu/cpu${i}/cpufreq/scaling_governor"
done

