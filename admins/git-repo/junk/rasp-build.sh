#!/bin/bash

for i in corba_pgenerators-testing corba_pgenerators corba_lpythonbp-admin corba_lpythonbp-admin-testing; do 
    for j in $(curl -s http://c.yandex-team.ru/api/groups2hosts/${i}); do 
	host -t A $j | grep "has address" | awk '{print $NF}';
	host -t AAAA $j | grep "has IPv6 address" | awk '{print $NF}'; 
    done;
done
