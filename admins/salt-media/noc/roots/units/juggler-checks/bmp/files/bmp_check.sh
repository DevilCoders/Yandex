#!/bin/bash

for service in $(echo "routinator pmbmpd bmp_analyzer bmp_importer upflow_manager"); do
	if systemctl -q is-active $service ; then
	       echo "PASSIVE-CHECK:$service-alive;0;OK"
	else
		echo "PASSIVE-CHECK:$service-alive;2;Failed"
	fi
done
