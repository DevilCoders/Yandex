{# Gobgp configuration #}
{% set hostname = grains['nodename'] %}
{% set host_roles = grains['cluster_map']['hosts'][hostname]['roles'] %}
{%- set host_tags = salt['grains.get']('cluster_map:hosts:%s:tags' % hostname) %}
{%- set host_roles = salt['grains.get']('cluster_map:hosts:%s:roles' % hostname) %}
{% set go2vpp_conf = salt['yavpp.go2vpp_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], grains) %}
{%- if 'bgp2vpp' in host_tags %}
{% set bgp2vpp_conf = salt['yavpp.bgp2vpp_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], grains) %}
{%- endif %}
{% set cgw_conf = salt['yavpp.cgw_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], host_tags, grains) %}
{% set vpp_conf = salt['yavpp.vpp_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], grains) %}
{% set new_vpp_conf = salt['yavpp.new_vpp_conf'](grains['cluster_map']['cloudgate_conf'][grains['nodename']], grains) %}



{# Go2VPP configuration #}
{% if 'cgw-dc' in host_tags or 'cgw-nat' in host_roles %}
gobgp-conf:
  file.managed:
    - name: /etc/gobgp/gobgpd.conf
    - source: salt://{{ slspath }}/tmpl/etc/gobgpd.conf.dc.j2
    - defaults: {{ cgw_conf }}
    - template: jinja
    - makedirs: True

go2vpp-conf:
  file.managed:
    - name: /etc/vpp/go2vpp.conf
    - source: salt://{{ slspath }}/tmpl/etc/go2vpp.conf.dc.j2
    - defaults: {{ cgw_conf }}
    - template: jinja
    - makedirs: True

{# VPP configuration #}
vpp-conf:
  file.managed:
    - name: /etc/vpp/startup.conf
    - source: salt://{{ slspath }}/tmpl/etc/startup.conf.j2
    - defaults: {{ new_vpp_conf }}
    - template: jinja

{# yavpp-configured startup scripts configuration #}
hostifaces-conf:
  file.managed:
    - name: /etc/vpp/hostifaces.conf
    - source: salt://{{ slspath }}/tmpl/etc/hostifaces.conf.dc.j2
    - defaults: {{ cgw_conf }}
    - template: jinja

{# yavpp vrfs configuration #}
cgw-vrfs-conf:
  file.managed:
    - name: /etc/vpp/cgw-vrfs.conf
    - source: salt://{{ slspath }}/tmpl/etc/cgw-vrfs.conf.dc.j2
    - defaults: {{ cgw_conf }}
    - template: jinja

{% else %}

{%-if 'bgp2vpp' in host_tags %}
{# bgp2vpp configuration #}
bgp2vpp-conf:
  file.managed:
    - name: /etc/bgp2vpp/config.yaml
    - source: salt://{{ slspath }}/tmpl/etc/bgp2vpp.conf.j2
    - defaults: {{ bgp2vpp_conf }}
    - template: jinja
    - makedirs: True
{%- else %}
{# Go2VPP configuration #}
go2vpp-conf:
  file.managed:
    - name: /etc/vpp/go2vpp.conf
    - source: salt://{{ slspath }}/tmpl/etc/go2vpp.conf.j2
    - defaults: {{ go2vpp_conf }}
    - template: jinja
    - makedirs: True
{%- endif %}

gobgp-conf:
  file.managed:
    - name: /etc/gobgp/gobgpd.conf
    - source: salt://{{ slspath }}/tmpl/etc/gobgpd.conf.j2
    - defaults: {{ go2vpp_conf }}
    - template: jinja
    - makedirs: True

{# yavpp-configured startup scripts configuration #}
hostifaces-conf:
  file.managed:
    - name: /etc/vpp/hostifaces.conf
    - source: salt://{{ slspath }}/tmpl/etc/hostifaces.conf.j2
    - defaults: {{ go2vpp_conf }}
    - template: jinja
    - makedirs: True

{# VPP configuration #}
vpp-conf:
  file.managed:
    - name: /etc/vpp/startup.conf
    - source: salt://{{ slspath }}/tmpl/etc/startup.conf.j2
    - defaults: {{ vpp_conf }}
    - template: jinja
{%- endif %}

{# yavpp-autorecovery configuration #}
autorecovery-conf:
  file.managed:
    - name: /etc/vpp/cgw-autorecovery.conf
    - makedirs: True
    - source: salt://{{ slspath }}/tmpl/etc/cgw-autorecovery.conf.j2

suicide-conf:
  file.managed:
    - name: /etc/vpp/cgw-suicide.conf
    - makedirs: True
    - source: salt://{{ slspath }}/tmpl/etc/cgw-suicide.conf.j2

{# dump VPP config info for loadbalancer to use #}
{% if 'loadbalancer-node' in host_roles %}
/etc/salt-lb/minion.d/vpp_conf.conf:
  file.serialize:
    - dataset:
        grains:
          common_salt:
            vpp_conf: {{ vpp_conf }}
    - formatter: yaml
    - makedirs: True
{% endif %}
