{% set vtype = salt['pillar.get']('data:dbaas:vtype') %}
{% if salt['pillar.get']('data:use_unbound_64', False) %}
unbound-config-local64:
    pkg.installed:
        - version: 0.7
{% else %}
local-named-package:
    pkg.installed:
        - name: config-caching-dns
        - version: 1.0-12
    service.running:
        - name: bind9
        - watch:
            - pkg: local-named-package
{% if vtype == 'compute' %}
            - cmd: reconfigure-eth0
{% endif %}

disable-caching-dns-cron:
    file.absent:
        - name: /etc/cron.d/config-caching-dns
        - require:
            - pkg: local-named-package
    service.running:
        - name: cron
        - watch:
            - file: disable-caching-dns-cron
{% if vtype != 'compute' %}
        - require_in:
            - file: /etc/resolv.conf
{% endif %}

{% if vtype != 'compute' %}
/etc/resolv.conf:
    file.managed:
        - template: jinja
        - source: salt://components/common/conf/etc/resolv.conf
        - mode: 644
        - user: root
        - follow_symlinks: False
        - require:
            - pkg: local-named-package
        - unless:
            - grep NAT64 /etc/resolv.conf
{% else %}

/etc/dhcp/forwarders:
    file.managed:
        - contents: {{ salt['pillar.get']('data:dns:forwarders', ['2a02:6b8::1:1', '2a02:6b8:0:3400::1']) | tojson }}

/etc/dhcp/forward_to_compute_zones:
{% if salt['pillar.get']('data:dns:forward_to_compute_zones') %}
    file.managed:
        - contents: {{ salt['pillar.get']('data:dns:forward_to_compute_zones') | join(' ') }}
        - watch_in:
            - cmd: reconfigure-eth0
{% else %}
    file.absent
{% endif %}

/etc/dhcp/dhclient-enter-hooks.d/dnsupdate:
    file.managed:
        - source: salt://components/common/conf/dnsupdate
        - mode: 755
        - require:
            - pkg: local-named-package
            - file: /etc/dhcp/forwarders

reconfigure-eth0:
    cmd.wait:
        # Ignore return code, cause MDB-8222
        - name: ifdown eth0 && ifup eth0 || true
        - watch:
            - file: /etc/dhcp/dhclient-enter-hooks.d/dnsupdate
            - file: /etc/dhcp/forward_to_compute_zones
{% endif %}
{% endif %}

{% if (not salt['pillar.get']('data:selfdns_disable', False)) and (salt['pillar.get']('data:l3host', False) or salt['pillar.get']('data:ipv6selfdns', False) or vtype != 'compute') %}
{% set selfdns_logfile = '/var/log/yandex-selfdns-client/client.log' %}
{%   if salt['pillar.get']('data:use_monrun', True) %}
include:
    - components.monrun2.selfdns
{%   endif %}
mdb-selfdns-token-pillar:
    test.check_pillar:
        - string:
              - data:selfdns-api:token

selfdns-client-package:
    pkg.installed:
    - name: yandex-selfdns-client
    - version: 0.2.18-9309118

/etc/yandex/selfdns-client/default.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/yandex/selfdns-client/default.conf
        - mode: 0644
        - user: root
        - group: selfdns
        - template: jinja
        - defaults:
            selfdns_logfile: {{ selfdns_logfile }}
        - require:
            - pkg: selfdns-client-package
            - test: mdb-selfdns-token-pillar

/etc/cron.d/yandex-selfdns-client:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/yandex-selfdns-client
        - mode: 644
        - template: jinja
        - defaults:
            selfdns_logfile: {{ selfdns_logfile }}
        - require:
            - pkg: selfdns-client-package

{% if salt['pillar.get']('data:second_selfdns', False)  %}
mdb-selfdns-token2-pillar:
    test.check_pillar:
        - string:
              - data:selfdns-api:token2

/etc/yandex/selfdns-client/second.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/yandex/selfdns-client/second.conf
        - mode: 644
        - template: jinja
        - defaults:
            selfdns_logfile: {{ selfdns_logfile }}
        - require:
            - pkg: selfdns-client-package
            - test: mdb-selfdns-token2-pillar

/etc/yandex/selfdns-client/plugins/second:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/yandex/selfdns-client/plugins/second
        - mode: 755
        - template: jinja
        - require:
            - pkg: selfdns-client-package
{% endif %}

{% if vtype == 'porto' %}
selfdns-client-plugins-mdb-porto-package:
    pkg.installed:
        - name: mdb-selfdns-client-plugins-mdb-porto
{%   if salt['pillar.get']('data:is_in_user_project_id', False) %}
        - version: 1.9694397
{%   else %}
        - version: 1.9556117
{%   endif %}
        - require:
            - pkg: selfdns-client-package
{% endif %}

/etc/yandex/selfdns-client/plugins/ipv6only:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/yandex/selfdns-client/plugins/ipv6only
        - mode: 755
        - template: jinja
        - require:
            - pkg: selfdns-client-package
{% else %}
selfdns-client-package:
    pkg.purged:
        - name: yandex-selfdns-client
        - require:
            - cmd: selfdns-client-stop

selfdns-client-stop:
    cmd.run:
        - name: pkill -u selfdns || true
        - onlyif:
            - pgrep -u selfdns
        - require:
            - file: /etc/cron.d/yandex-selfdns-client

/etc/cron.d/yandex-selfdns-client:
    file.absent
{% endif %}


{% if not salt['pillar.get']('data:use_unbound_64', False) %}
/etc/systemd/system/bind9.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/bind9.service
        - mode: 644
        - require:
            - pkg: local-named-package
        - require_in:
            - service: local-named-package
        - onchanges_in:
            - module: systemd-reload
{% endif %}

dnsutils-package:
    pkg.installed:
        - name: dnsutils
