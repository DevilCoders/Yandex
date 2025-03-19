#!/bin/bash

ETHS=$(ls -1 /sys/class/net/ | egrep "eth[0-9]*")
NETCONF="/etc/network/interfaces"
UDEVCONFS="/etc/udev/rules.d/*net.rules"
NEWDUMPIF="eth9"
RIF="eth0"

CONFIF=$(egrep -o "auto eth[0-9]*" $NETCONF | awk '{print $NF}')
if [[ $CONFIF != $RIF ]]; then
  RIF=$CONFIF
fi

if (( $(ip -o a l scope global | grep eth | wc -l) > 0 )); then
  exit 0
fi

declare -a AETHS
for i in $ETHS; do
  if (( $(cat /sys/class/net/$i/carrier_up_count) > 0 )); then
    AETHS+=($i);
  fi
done

ACTUALIF=""

# hack for case when no carrier_up_count but link is actualy aviable
if (( ${#AETHS[@]} == 0 )); then
  for i in $ETHS; do
    /sbin/ifconfig $i up 2>/dev/null 1>/dev/null
  done
  if (( $(cat /sys/class/net/$i/carrier_up_count) > 0 )); then
    AETHS+=($i);
  fi
fi

if (( ${#AETHS[@]} > 1 )); then
  ACTUALIF=$(for i in eth2; do echo `ethtool $i | grep Speed: | egrep -o "[0-9]*"`" "$i; done | sort -n -r | awk '{print $NF}')
elif (( ${#AETHS[@]} == 1 )); then
  ACTUALIF=${AETHS[0]}
else
  exit 0
fi

if [[ $CONFIF == $ACTUALIF ]]; then
  exit 0;
fi

sed -i "s/$CONFIF/$NEWDUMPIF/g" $UDEVCONFS
sed -i "s/$ACTUALIF/$RIF/g" $UDEVCONFS
sync
sleep 1
/sbin/reboot

