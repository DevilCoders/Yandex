#! /bin/bash

# cleanup
tc qdisc del dev eth0 root

# rules
tc qdisc add dev eth0 root handle 1: htb default 11

tc class add dev eth0 parent 1: classid 1:1 htb rate 50mbps ceil 50mbps 
tc class add dev eth0 parent 1:1 classid 1:10 htb rate 25mbps ceil 50mbps
tc class add dev eth0 parent 1:1 classid 1:11 htb rate 25mbps ceil 50mbps

tc qdisc add dev eth0 parent 1:10 handle 10: sfq perturb 5 quantum 5000 limit 1024
tc qdisc add dev eth0 parent 1:11 handle 11: sfq perturb 5 quantum 5000 limit 1024

# and filters
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 213.180.192.0/19 flowid 1:10 
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 87.250.224.0/19 flowid 1:10 
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.0.0/18 flowid 1:10 
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 93.158.128.0/18 flowid 1:10 
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 95.108.128.0/17 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 199.36.240.0/22 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 199.21.96.0/22 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 178.154.128.0/17 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 141.8.128.0/18 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 130.193.32.0/19 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 100.43.64.0/19 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 84.201.150.0/23 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 84.201.128.0/18 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.4.0/23 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.6.32/27 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.6.64/26 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.6.128/25 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.7.0/24 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.8.0/21 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.16.0/20 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 77.88.32.0/19 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 37.140.128.0/18 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 37.9.64.0/18 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 5.255.192.0/18 flowid 1:10
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst 5.45.192.0/18 flowid 1:10
