#!/bin/bash
#
# Provides: expander_firmware

me=${0##*/}     # strip path
me=${me%.*}     # strip extension

die() {
        echo "$1;$2"
        exit 0
}

ExpanderDevice=`/usr/bin/xflash -i get avail | grep SAS2x28 | grep 0.0.0.0 | awk '{print $4}' | sed 's/(//g' | sed 's/)//g' | sed 's/://g' 2>/dev/null`
if [ -z "$ExpanderDevice" ]; then
    die "0" "OK";
fi

VersionCurrent=`/usr/bin/xflash -y -i $ExpanderDevice get ver | grep Firmw | grep Ver | awk '{print $4}' 2>/dev/null`
if [ -z "$VersionCurrent" ]; then
    die "0" "OK";
fi
VersionNeeded=01.11.09.56

if [ "x$VersionCurrent" == "x$VersionNeeded" ] 
then
    die "0" "OK";
else
    die "1" "Expander has wrong firmware: $VersionCurrent, Needed: $VersionNeeded";
fi
