#!/usr/bin/python
# -*- coding: utf-8 *-*
try:
	import commands
	HDD_Max_Temp=float(commands.getstatusoutput("cat /var/tmp/disk_temp/disk_temp_* 2>/dev/null | sort | tail -1")[1])
	HDD_Avg_Temp=float(commands.getstatusoutput("cat /var/tmp/disk_temp/disk_temp_* 2>/dev/null | awk '{sum+=$1} END {print sum/NR}'")[1])
	HDD_Sys_Temp=float(commands.getstatusoutput("cat /proc/mdstat | grep 'md1 ' | egrep -oh 'sd[a-z]+' | while read TuschechnyiDisk; do cat /var/tmp/disk_temp/disk_temp_$TuschechnyiDisk 2>/dev/null; done | awk '{sum+=$1} END {print sum/NR}'")[1])
	result = [ ("HDD_Max_Temp" , HDD_Max_Temp), ("HDD_Avg_Temp",HDD_Avg_Temp), ("HDD_Sys_Temp",HDD_Sys_Temp)]
	for x in result:
	    print x[0],x[1]

	try:
	    CPU_Avg_Temp=float(commands.getstatusoutput("grep -i cpu /var/tmp/ipmi_sensor | grep degrees | awk 'BEGIN { FS = \"|\" } ; {sum+=$2} END {print sum/NR}'")[1])
	    print "CPU_Avg_Temp", CPU_Avg_Temp
	except:
	    pass

	try:
	    SYS_Temp=float(commands.getstatusoutput("grep -i sys /var/tmp/ipmi_sensor | grep degrees | awk 'BEGIN { FS = \"|\" } ; { print $2 }'")[1])
	    print "SYS_Temp",SYS_Temp
	except:
	    pass
except:
        pass
