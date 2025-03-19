{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

/usr/bin/wd-service.sh:
    file.absent

{% for srv in mongodb.services_deployed %}
{%   set config = mongodb.config.get(srv) %}
{%   set service = config._srv_name %}

/etc/cron.d/wd-{{service}}:
    file.absent
{% endfor %}
