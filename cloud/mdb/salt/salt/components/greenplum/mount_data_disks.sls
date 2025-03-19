{% from "components/greenplum/map.jinja" import sysvars with context %}
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
{% set role = 'master' %}
{% else %}
{% set role = 'segment' %}
{% endif %}

{% if salt.pillar.get('data:fscreate', True) %}
{%   if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False)  and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
{%     if sysvars.fs == 'xfs' %}
xfsprogs:
  pkg.installed
{%     endif %}

{%     for disk,disk_params in salt.pillar.get('data:data_disks:'+role).items()|sort %}
Create-fs-on-{{ disk }}:  
  module.run:
{% if sysvars.fs == 'xfs' %}
    - name: {{ sysvars.fs }}.mkfs
{% else %}
    - name: extfs.mkfs
    - fs_type: {{ sysvars.fs }}
{% endif %}
    - device: {{ disk }}
    - noforce: True
    - label: {{ disk_params['label'] }}
    - unless: "blkid {{ disk }} | grep {{ sysvars.fs }}"

Mount-{{ disk }}-to-{{ disk_params['mount_dir'] }}:
  mount.mounted:
    - name: {{ disk_params['mount_dir'] }}
    - device: LABEL={{ disk_params['label'] }}
    - fstype: {{ sysvars.fs }}
    - mkmnt: True
    - opts: {{ disk_params['mount_opts'] }}
    - persist: True
    - mount: True  
{%     endfor %}
{%   endif %}
{% endif %}
