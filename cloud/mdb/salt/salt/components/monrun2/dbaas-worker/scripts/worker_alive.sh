#!/bin/bash

{% if salt['pillar.get']('data:disable_worker', False) %}
echo '0;OK'
{% else %}
if /usr/bin/supervisorctl status dbaas-worker 2>/dev/null | grep -q RUNNING
then
    echo '0;OK'
else
    echo '2;dbaas-worker is not running'
fi
{% endif %}
