#!/bin/bash

check='pull-cluster-map'
grains_err_file='/etc/salt/grains.err'
grains_new_file='/etc/salt/grains.new'
grains_new_epoch_file=$grains_new_file.epoch

if [ -f "$grains_err_file" ] ; then
    echo "PASSIVE-CHECK:$check;2;$(cat $grains_err_file)"
else
    if [ -f "$grains_new_epoch_file" ] ; then
        echo "PASSIVE-CHECK:$check;0;$(cat $grains_new_epoch_file)"
    else
        echo "PASSIVE-CHECK:$check;0;No updates"
    fi
fi
