#!/bin/bash -e

{%- set hostname = grains['nodename'] %}
{%- set disk_set_name = grains['cluster_map']['hosts'][hostname]['kikimr']['disk_set']  -%}
{%- set disk_set = grains['cluster_map']['kikimr']['disk_sets'][disk_set_name] %}

{%- for disk in disk_set %}
touch {{ disk.path }}
chown kikimr {{ disk.path }}
DISK_SIZE={{ salt['grains.get']("cluster_map:kikimr:drive_size", "100G") }}
/usr/bin/truncate -s "$DISK_SIZE" {{ disk.path }}
{%- endfor %}
