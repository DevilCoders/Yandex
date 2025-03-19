{% set ufm_host_type = salt['grains.filter_by'](pillar['ufm_hosts'], grain='fqdn') %}
{% set ufm_host_conf = pillar['ufm_srv_types'][ufm_host_type] %}


/usr/local/bin/update_opensm_conf.sh:
  file.managed:
    - source: salt://files/nocdev-ufm/usr/local/bin/update_opensm_conf.sh
    - makedirs: True
    - mode: 0755
    - user: root

{% if ufm_host_type in ["opensm", "baremetal"] %}

{{ufm_host_conf.opensm_conf}}:
  file.managed:
    - source: salt://files/nocdev-ufm/{{grains['fqdn']}}/opensm.conf
    - template: jinja
    - makedirs: True

"Generate {{ufm_host_conf.root_guid_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{ufm_host_conf.root_guid_conf}} --gen-root-guid --salt-output
    - cwd: /tmp
    - stateful: True

{% endif %}

{% if ufm_host_type in ["docker"] %}
/var/cache/ufm-salt/opensm.conf:
  file.managed:
    - source: salt://files/nocdev-ufm/{{grains['fqdn']}}/opensm.conf
    - template: jinja
    - makedirs: True

"Copy opensm.conf to docker://ufm:{{ufm_host_conf.opensm_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{ufm_host_conf.opensm_conf}} -d ufm -f /var/cache/ufm-salt/opensm.conf --salt-output
    - cwd: /tmp
    - stateful: True

"Generate docker://ufm:{{ufm_host_conf.root_guid_conf}}":
  cmd.run:
    - name: /usr/local/bin/update_opensm_conf.sh -c {{ufm_host_conf.root_guid_conf}} -d ufm --gen-root-guid --salt-output
    - cwd: /tmp
    - stateful: True

{% endif %}


