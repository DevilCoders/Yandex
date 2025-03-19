{%- import "common/kikimr/init.sls" as kikimr_vars with context %}

include:
  - nginx

{% set hostname = grains['nodename'] %}
{% set default_zone_id = pillar['placement']['dc'] %}
{% set zone_id = salt['grains.get']('cluster_map:hosts:%s:location:zone_id' % hostname, default_zone_id) %}
{% set nbs_cluster = salt['grains.get']('cluster_map:hosts:%s:kikimr:nbs_cluster_id' % hostname) %}
{% set nbs_host = salt['grains.get']('cluster_map:kikimr:clusters:%s:dynamic_nodes:%s' % (nbs_cluster, kikimr_vars.nbs_database), ['localhost'])|random %}

/etc/yc/compute/config.yaml:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://{{ slspath }}/files/config.yaml
    - require:
      - yc_pkg: yc-compute
    - context:
      nbs_host: {{ nbs_host }}
      zone_id: {{ zone_id }}

/etc/yc/compute/certnew.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub.crt
    - replace: False
    - mode: 0600
    - user: yc-compute
    - require:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config.yaml

/etc/yc/compute/intranet_token:
  file.managed:
    - contents: 'intranet token stub'
    - replace: False
    - mode: 0600
    - user: yc-compute

/etc/yc/compute/privkey.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/stub.key
    - replace: False
    - mode: 0600
    - user: yc-compute
    - require:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config.yaml

/var/lib/yc/compute/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/files/system_account.json
    - mode: '0040'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-compute
    - require:
      - yc_pkg: yc-compute

yc-compute:
  yc_pkg.installed:
    - pkgs:
      - yc-compute
      - python-contrail

{% set roles = salt['grains.get']('cluster_map:hosts:%s:roles' % hostname) %}
{% set base_role = salt['grains.get']('cluster_map:hosts:%s:base_role' % hostname) %}

{% if 'seed' not in roles %}
yc-compute-worker:
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config.yaml
    - watch:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config.yaml
{% endif %}

{% if 'seed' in roles or base_role == 'cloudvm' %}
populate-devel-database:
  cmd.run:
    - name: /usr/bin/yc-compute-populate-devel-database
    - require:
      - file: /etc/yc/compute/config.yaml
    - onchanges:
      - yc_pkg: yc-compute
    - require_in:
      - service: yc-compute-api

{% for platform in grains['platforms'][grains['cluster_map']['environment']] %}
platform-{{ platform['id'] }}:
  cmd.run:
    - name: '/usr/bin/yc-compute-admin platforms register --force <(cat)'
    - shell: /bin/bash
    - stdin: "{{ platform | yaml() }}"
    - env:
      - LC_ALL: C.UTF-8
      - LANG: C.UTF-8
    - require:
      - service: yc-compute-api
{% endfor %}
{% endif %}

yc-compute-api:
  service.running:
    - enable: True
    - require:
      - yc_pkg: yc-compute
      - file: /etc/yc/compute/config.yaml
      - file: /var/lib/yc/compute/system_account.json
    - watch:
      - file: /etc/yc/compute/config.yaml
      - yc_pkg: yc-compute

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
