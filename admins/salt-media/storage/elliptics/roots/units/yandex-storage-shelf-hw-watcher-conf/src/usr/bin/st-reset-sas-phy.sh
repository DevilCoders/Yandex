#!/bin/bash

reset_phy () {
        # find SAS address
	# sg_ses -p 0xa /dev/sg29
        # Element index: 39
        #   Transport protocol: SAS
        #   number of phys: 1, not all phys: 0, device slot number: 40
        #   phy index: 0
        #     device type: no device attached
        #     initiator port for:
        #     target port for: SATA_device
        #     attached SAS address: 0x50015b21f101927f
        #     SAS address: 0x50015b21f101924e !!!!!
        #     phy identifier: 0x0
	address=`sg_ses -p 0xa $1 | grep -P "Element index: $2$" -A9 |grep -P '^[ \t]+SAS address:' | awk '{print $(NF)}' | sed 's/0x//g'`
	if [[ -z $address ]]; then exit 3; fi

        # find port
        # ioc0              LSI Logic 0086 01        MPT 200   Firmware 0f000000   IOC 0
        #  0   9   0  Disk       ATA      ST4000NM0033-9ZM SN03  50015b2148079f32    18
        # ioc1              LSI Logic 0086 01        MPT 200   Firmware 0f000000   IOC 0
        port_name=`lsiutil -s |grep -P "(${address}|ioc)" | grep "${address}" -B1 | grep -Po "^ioc\d+"`
        #  1.  ioc0              LSI Logic 0086 01         200      0f000000     0
	#  2.  ioc1              LSI Logic 0086 01         200      0f000000     0
        port_num=`echo -e '0\n' | lsiutil  | grep " ${port_name} " | awk '{print $1}' | sed 's/\.//g'`
	if [[ -z $port_num ]]; then exit 3; fi

        # find PhyNum and Parent
        line=`lsiutil -p $port_num 16 | grep -P "$address"`
        #  B___T     SASAddress     PhyNum  Handle  Parent  Type
        #  0  65  50015b21f1019263    35     0049    000b   SATA Target
        Phy=`echo $line | awk '{print $4}'`
        Parent=`echo $line | awk '{print $6}'`

        if [[ ! -z $Phy ]] && [[ ! -z $Parent ]]
	then
                # stop
                echo -e "${Parent}\n${Phy}\n" | lsiutil -p $port_num 80 >/dev/null
                sleep 5
                # start
                echo -e "${Parent}\n${Phy}\n" | lsiutil -p $port_num 81 >/dev/null
                sleep 5
	else
		exit 2
	fi
		
}

if [ -e /usr/bin/shelf_tool ]
then
	disk_name=`shelf_tool |grep -P "(^[ \t]+$2|/dev/sg\d+)" | grep "$1" -A1 | grep -P "sd[\w\d]+" | awk '{print $4}'`
	if [ ! -z $disk_name ] 
	then
		echo "Phy in system"
		exit 0
	fi
fi
reset_phy $1 $2

if [ -e /usr/bin/shelf_tool ]
then
	if [[ `shelf_tool |grep -P "(^[ \t]+$2|/dev/sg\d+)" | grep "$1" -A1 | grep -P "sd[\w\d]+"` ]]
	then
		echo "OK"
		exit 0
	else
		echo "Fail"
		exit 1
	fi
fi
