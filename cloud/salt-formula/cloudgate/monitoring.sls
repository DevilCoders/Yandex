{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}

yc-monitors:
  yc_pkg.installed:
    - pkgs:
      - yc-autorecovery
      - yc-monitors

{# TODO: remove after CLOUD-19871 is deployed #}
{% set old_mon_bin_list = ["bug-common-vpp.sh", "cgw-announces.py", "cgw-daemons.sh", "cgw-fibdump.sh", "cgw-fibsync.py", "cgw-gretuns.sh", "cgw-ribstats.py", "ipfilter.py", "vpp-common.sh", "arp-ipv4-vpp.bash", "arp-ipv4-vpp.bash"] %}
{% set old_mon_etc_list = ["arp-ipv4-v4-vpp.conf", "arp-ipv4-v6-vpp.conf", "bug-common-v4-vpp.conf", "bug-common-v6-vpp.conf", "bug-common-vpp.conf", "cgw-fibdump.conf", "cgw-fibsync.conf", "faulty-default-v4-vpp.conf", "faulty-default-v6-vpp.conf", "label16-v4-vpp.conf", "label16-v6-vpp.conf", "unset-v4-vpp.conf", "unset-v6-vpp.conf", "arp-ipv4-vpp.conf", "unset-vpp.conf", "vpp-common.sh", "cgw-announces.yaml"] %}
{% set old_cgw_checks_list = ["arp-ipv4-v4-vpp", "arp-ipv4-v6-vpp",  "faulty-default-v4-vpp", "faulty-default-v6-vpp", "label16-v4-vpp", "label16-v6-vpp", "unset-v4-vpp", "unset-v6-vpp"] %}
remove-old-cgw-checks:
  file.absent:
    - names:
{%- for name in old_cgw_checks_list %}
      - /etc/monrun/conf.d/{{name}}.conf
{%- endfor %}

{{slspath}}-regenerate-monrun-checks:
  cmd.run:
     - name: /usr/bin/monrun --gen-jobs
     - onchanges:
       - file: remove-old-cgw-checks

remove-old-cgw-checks-files:
  file.absent:
    - names:
{%- for name in old_mon_bin_list %}
      - /home/monitor/agents/modules/{{ name }}
{%- endfor %}
{%- for name in old_mon_etc_list %}
      - /home/monitor/agents/etc/{{ name }}
{%- endfor %}
    - require:
      - cmd: {{slspath}}-regenerate-monrun-checks


{%- set hostname = grains['nodename'] %}
{%- set cluster_map = grains['cluster_map'] %}
{%- set cloudgate = cluster_map['cloudgate_conf'][hostname] %}
{%- set downstream = cloudgate['downstream'] %}
{%- set upstream4 = cloudgate.get('upstream4', {}) %}
{%- set reflector = cloudgate.get('reflector', {}) %}

{%- for fqdn in (cluster_map['roles'].get('oct_ctrl', []) + cluster_map['roles'].get('oct_head', [])) | unique %}
{{ fqdn }}-host:
  host.present:
    - name: {{ fqdn }}
    - ip: {{ cluster_map['hosts'][fqdn]['ipv4']['addr'] }}
{%- endfor %}

{%- for peer in upstream4.get('peer_addresses', []) %}
{{ peer }}-host:
  host.present:
    - name: ToR
    - ip: {{ peer }}
{%- endfor %}

{%- for peer in reflector.get('peer_addresses', []) %}
{{ peer }}-host:
  host.present:
    - name: reflector
    - ip: {{ peer }}
{%- endfor %}
