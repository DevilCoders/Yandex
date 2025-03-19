{% from "components/greenplum/map.jinja" import dep,sysvars with context %}
{% set iface_channels = '4' %}

{% if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False)  and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
{%   if salt.pillar.get('data:net_iface_tune', True) -%}
{%     for iface in grains['ip_interfaces'] %}
{%       if iface != "lo" %}
tune-net-iface-{{ iface }}-persistent:
  file.append:
    - name: {{ dep.rc_local }}
    - text: 
      - "/sbin/ifconfig {{ iface }} mtu {{ sysvars.iface_mtu }}"
      - "/sbin/ifconfig {{ iface }} txqueuelen {{ sysvars.iface_txql }}"

'/sbin/ifconfig {{ iface }} mtu {{ sysvars.iface_mtu }}':
  cmd.run:
    - unless: /sbin/ifconfig {{ iface }} | egrep -q 'mtu\s+{{ sysvars.iface_mtu }}'
  
'/sbin/ifconfig {{ iface }} txqueuelen {{ sysvars.iface_txql }}':
  cmd.run:
    - unless: /sbin/ifconfig {{ iface }} | egrep -q 'txqueuelen\s+{{ sysvars.iface_txql }}'

{%         if salt['grains.get']('num_cpus') > 4 %}
set-channel-{{ iface }}-persistent:
  file.append:
    - name: {{ dep.rc_local }}
    - text:
      - "/sbin/ethtool -L {{ iface }} combined {{ iface_channels }}"

'/sbin/ethtool -L {{ iface }} combined {{ iface_channels }}':
  cmd.run:
    - unless: /sbin/ethtool -l {{ iface }} | awk '/Current hardware settings/,/^$/' | egrep -qi '^combined:\s+{{ iface_channels }}$'
{%         endif %}
{%       endif %}
{%     endfor %}
{%   endif %}
{% endif %}
