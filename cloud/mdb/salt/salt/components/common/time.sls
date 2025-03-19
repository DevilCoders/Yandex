{% set vtype = salt['pillar.get']('data:dbaas:vtype', 'porto') %}
{% if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False) and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
local-ntp-config:
    pkg.installed:
        - name: config-yabs-ntp
        - version: 0.1-18

{%     if salt['pillar.get']('data:ntp:servers') %}
/etc/ntp.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/ntp.conf
        - template: jinja
        - mode: 644
        - user: root
        - group: root
        - require:
              - pkg: local-ntp-config
        - require_in:
              - service: local-ntp-service
{%     endif %}

local-ntp-service:
    service.running:
        - name: ntp
        - require:
            - pkg: local-ntp-config
{% endif %}

tzdata:
    pkg.installed:
        - version: 2020e-0~yandex~ubuntu18.04

{% if vtype == 'compute' %}
/etc/dhcp/dhclient-exit-hooks.d/ntp:
    file.absent

/etc/dhcp/dhclient-exit-hooks.d/ntpdate:
    file.absent

/run/ntp.conf.dhcp:
    file.absent:
        - watch_in:
            - service: local-ntp-service
{% endif %}

/etc/cron.d/wd-ntp-qemu:
    file.absent

/etc/cron.d/wd-ntp:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/cron.d/wd-ntp
        - mode: 644
        - user: root
        - group: root

/etc/cron.yandex/wd-ntp.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/etc/wd-ntp.sh
        - mode: 755
        - user: root
        - group: root
        - require:
            - file: /etc/cron.yandex

/etc/cron.d/config-yabs-ntp:
    file.absent

# http://www.freedesktop.org/software/systemd/man/localtime.html
/etc/localtime:
    file.symlink:
        - target: /usr/share/zoneinfo/Europe/Moscow
        - force: True
        - require:
            - pkg: tzdata
