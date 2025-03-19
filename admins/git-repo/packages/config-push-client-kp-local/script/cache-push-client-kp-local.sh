#!/bin/bash

#hostname
H=`hostname -f`
curl --connect-timeout 10 -s -o /var/tmp/$H http://c.yandex-team.ru/api-cached/generator/paulus.aggregation_group?fqdn=$H || (echo "can't connect to conductor"; exit)
