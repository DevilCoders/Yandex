#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

{% if salt['pillar.get']('data:salt_masterless') %}
die 0 "Nop. Salt Masterless"
{% else %}
if sudo service salt-minion status 2>/dev/null | grep -q running 2>/dev/null
then
    die 0 "OK"
else
    die 2 "Service is dead"
fi
{% endif %}
