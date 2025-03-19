#!/bin/bash

. /usr/local/sbin/autodetect_active_eth

if [ $buggy_nic -eq 1 ]; then
        echo "2;buggy nic";
elif
	[ $is_virtual_host -eq 1 ] && [ $is_openvz_host -eq 1 ] ; then
                echo "0;OK, openvz CT, skip buggy_nic checking"
elif
        [ $is_virtual_host -eq 1  ] && [ $is_lxc_host -eq 1 ]; then
                echo "0;OK, lxc CT, skip buggy_nic checking"
else
	echo "0;Ok";
fi
