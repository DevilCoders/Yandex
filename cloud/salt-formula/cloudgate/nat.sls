{# Now we are using mostly cloudgate config with checks for cgw-nat role in salt roles. Here will be placed only nat specific stuff.#}
# CLOUD-15512: move this to all packages when merge
{% set hostname = grains['nodename'] %}
{%- set host_tags = salt['grains.get']('cluster_map:hosts:%s:tags' % hostname) %}
{% set cgw_conf = salt['yavpp.cgw_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], host_tags, grains) %}
{%- set service = "yavpp-nat" %}

nat-conf:
  file.managed:
    - name: /etc/vpp/nat.conf
    - source: salt://{{ slspath }}/tmpl/etc/nat.conf.j2
    - defaults: {{ cgw_conf }}
    - template: jinja
    - makedirs: True
    - require_in:
        - service: vpp-service
    - watch_in:
        - service: vpp-service

{{ service }}-conf-file:
  file.managed:
    - name: /etc/systemd/system/yavpp-configured.service.d/10-yavpp-nat.conf
    - source: salt://{{ slspath }}/tmpl/vpp-configured-service-conf/10-yavpp-nat.conf.j2
    - defaults: {{ cgw_conf }}
    - template: jinja
    - makedirs: True
    - require_in:
        - service: vpp-service
    - watch_in:
        - service: vpp-service

#TODO: must be fixed in CLOUD-22375
daemonreload:
  cmd.run:
    - name: systemctl daemon-reload
    - onchanges:
        - file: {{service}}-conf-file
    - require_in:
        - service: vpp-service
