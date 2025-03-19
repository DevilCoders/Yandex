{%- set vpp_conf = salt['yavpp.vpp_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], grains) %}

{%- for conffile in ('10-yavpp-environment', '20-yavpp-hugepages', '30-yavpp-bindings') %}
{{ conffile }}-conf-file:
  file.managed:
    - name: /etc/systemd/system/vpp.service.d/{{ conffile }}.conf
    - source: salt://{{ slspath }}/tmpl/vpp-service-conf/{{ conffile }}.conf.j2
    - defaults: {{ vpp_conf }}
    - template: jinja
    - makedirs: True
    - watch_in:
      - cmd: daemon_reload_cgw
{%- endfor %}

{%- if not grains['is_mellanox'] %}
25-yavpp-ifaces-conf-file:
  file.managed:
    - name: /etc/systemd/system/vpp.service.d/25-yavpp-ifaces.conf
    - source: salt://{{ slspath }}/tmpl/vpp-service-conf/25-yavpp-ifaces.conf.j2
    - defaults: {{ vpp_conf }}
    - template: jinja
    - makedirs: True
    - watch_in:
      - cmd: daemon_reload_cgw
{%- endif %}

{%- set hostname = grains['nodename'] %}
{%- set host_tags = salt['grains.get']('cluster_map:hosts:%s:tags' % hostname) %}
{%- set bgp_vpp_converter = 'go2vpp' %}
{%-if 'bgp2vpp' in host_tags %}
{%- set bgp_vpp_converter = 'bgp2vpp' %}
{%- endif %}
{%- set common_services = ('yavpp-stats', 'yavpp-gobgp-stats', 'yavpp-gobgp-rtt', 'yavpp-configured', 'gobgp', 'yavpp-config-ensure-on-start', bgp_vpp_converter) %}
{%- set conf_services = common_services + ('yavpp-autorecovery',) %}
{%- set mon_services = common_services + ('vpp', 'yavpp-autorecovery.timer') %}
{%- for serv in conf_services %}
{{ serv }}-service-file:
  file.managed:
    - name: /etc/systemd/system/{{ serv }}.service
    - source: salt://{{ slspath }}/tmpl/service/{{ serv }}.service.j2
    - defaults: {{ vpp_conf }}
    - template: jinja
    - makedirs: True
{%- endfor %}

{#- Fill it here to have only one place with service list #}
/home/monitor/agents/etc/cgw-daemons.conf:
  file.managed:
    - makedirs: True
    - contents: |
        services:
{%- for serv in mon_services %}
          - {{ serv }}
{%- endfor %}

{%- set fqdn = grains['nodename'] %}
{%- set cluster_map = grains['cluster_map'] %}
{%- set cluster_id = cluster_map['hosts'][fqdn]['oct']['cluster_id'] %}
{%- set local_cgws = cluster_map['oct']['clusters'][cluster_id]['cloudgates'] %}
{%- set period = 10 %}

{%- for cgw in local_cgws %}
  {%- if cgw == fqdn %}
yavpp-autorecovery-timer-file:
  file.managed:
    - name: /etc/systemd/system/yavpp-autorecovery.timer
    - source: salt://{{ slspath }}/tmpl/service/yavpp-autorecovery.timer.j2
    - template: jinja
    - makedirs: True
    - defaults:
        offset: {{ (loop.index0 * period/loop.length)|round|int }}
        period: {{ period }}
    - onchanges_in:
      - cmd: daemon_reload_cgw
  {%- break %}
  {%- endif %}
{%- endfor %}

yavpp-vf-service-file-remove-old:
  file.absent:
    - name: /etc/systemd/system/yavpp-vf.service

daemon_reload_cgw:
  cmd.run:
    - name: systemctl daemon-reload
    - onchanges:
       {%- for service in conf_services %}
        - file: {{service}}-service-file
       {%- endfor %}
    - require_in:
        - service: vpp-service
