{% set osm_host_type = salt['grains.filter_by'](pillar['osm_hosts'], grain='fqdn') %}
{% set osm_host_conf = pillar['osm_srv_types'][osm_host_type] %}

{% set var_dirs = [
  "/var/cache/ib-mon",
  "/var/log/ib-mon",
  "/var/log/ib-mon-export-metrics",
  "/var/log/ib-mon-export-switch-state"] %}
{% for dir in var_dirs %}
{{dir}}:
  file.directory:
    - makedirs: True
    - user: telegraf
    - mode: 0755
{% endfor %}

/var/log/ib-mon-export-ports:
  file.directory:
    - makedirs: True
    - user: robot-racktables
    - mode: 0755

/etc/ib-mon.conf:
  file.managed:
    - template: jinja
    - makedirs: True
    - source: salt://files/noc-osm/etc/ib-mon.conf
    - group: telegraf
    - mode: 640

{% set configs = ["etc/cron.d/ib-mon", "etc/logrotate.d/ib-mon", "etc/sudoers.d/ib-mon"] %}
{% for file in configs %}
/{{file}}:
  file.managed:
    - makedirs: True
    - source: salt://files/noc-osm/{{file}}
{% endfor %}

/etc/telegraf/telegraf.d/ib-mon.conf:
  file.managed:
    - makedirs: True
    - source: salt://files/noc-osm/etc/telegraf/telegraf.d/ib-mon.conf
  service.running:
    - name: telegraf
    - restart: True
    - require:
      - file: /etc/telegraf/telegraf.d/ib-mon.conf
    - watch:
      - file: /etc/telegraf/telegraf.d/ib-mon.conf
  pkg:
    - installed
    - pkgs:
      - telegraf-noc-conf
      - telegraf

/opt/ib-mon/:
  file.recurse:
    - source: salt://files/noc-osm/opt/ib-mon/
    - file_mode: keep

/usr/local/bin/update_opensm_conf.sh:
  file.managed:
    - source: salt://files/noc-osm/usr/local/bin/update_opensm_conf.sh
    - makedirs: True
    - mode: 0755
    - user: root

{% if grains['host'] != 'sas2-10-ufm-master' %}
/opt/ib-mon/var/no-ufm:
  file.managed:
    - makedirs: True
{% endif %}

{% if osm_host_type in ["opensm", "baremetal"] %}

{{osm_host_conf.opensm_conf}}:
  file.managed:
    - source: salt://files/noc-osm/{{grains['fqdn']}}/opensm.conf
    - template: jinja
    - makedirs: True

"Generate {{osm_host_conf.root_guid_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{osm_host_conf.root_guid_conf}} --gen-root-guid --salt-output
    - cwd: /tmp
    - stateful: True

{% endif %}

{% if osm_host_type in ["docker"] %}
/var/cache/ufm-salt/opensm.conf:
  file.managed:
    - source: salt://files/noc-osm/{{grains['fqdn']}}/opensm.conf
    - template: jinja
    - makedirs: True

"Copy opensm.conf to docker://ufm:{{osm_host_conf.opensm_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{osm_host_conf.opensm_conf}} -d ufm -f /var/cache/ufm-salt/opensm.conf --salt-output
    - cwd: /tmp
    - stateful: True

"Generate docker://ufm:{{osm_host_conf.root_guid_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{osm_host_conf.root_guid_conf}} -d ufm --gen-root-guid --salt-output
    - cwd: /tmp
    - stateful: True

{% endif %}
