%r:vlan-switch! 542

%r:ipmi-boot! pxe
%^setup

clearpart --all

echo "zero-fill 1"
for i in `ls /dev/sd*  | grep -v "[0-9]"`; do dd if=/dev/zero of=${i} bs=1024000; done

echo "zero-fill 2"
for i in `ls /dev/sd*  | grep -v "[0-9]"`; do dd if=/dev/zero of=${i} bs=1024000; done

reboot

%r:ipmi-boot! disk
