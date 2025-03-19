{% from "components/mongodb/walg/map.jinja" import walg with context %}
{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set config = mongodb.config.get('mongos') %}

{% set stop_local_minutes = walg.backup_local_minutes - 30 %}
{% set stop_hours = (24 + stop_local_minutes // 60) % 24 %}
{% set stop_minutes =  stop_local_minutes % 60 %}

{% set stop_duration_minutes = config._settings.balancerWindowStopDurationMinutes %}
{% set start_hours = (24 + (stop_local_minutes + stop_duration_minutes) // 60) % 24 %}
{% set start_minutes = (stop_local_minutes + stop_duration_minutes) % 60 %}

{% set stop = '{:02}:{:02}'.format(stop_hours, stop_minutes) %}
{% set start = '{:02}:{:02}'.format(start_hours, start_minutes) %}
mongos-set-balancer-active-window:
    mdb_mongodb.ensure_balancer_active_window:
      - service: mongos
      - stop: '{{ stop }}'
      - start: '{{ start }}'
