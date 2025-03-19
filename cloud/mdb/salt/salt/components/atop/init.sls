{% if salt['pillar.get']('data:use_atop', True) %}

/etc/cron.d/atop:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/atop.cron
        - mode: 644
        - user: root
        - group: root
        - require:
            - pkg: atop-package

/etc/atop/atop.daily:
    file.absent

/usr/share/atop/atop.daily:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/atop.daily
        - mode: 755
        - user: root
        - group: root
        - makedirs: True
        - require:
            - pkg: atop-package

/etc/init.d/atopacct:
    file.absent

/lib/systemd/system/atopacct.service:
    file.absent:
        - onchanges_in:
            - module: systemd-reload
        - require:
            - service: atopacct
            - file: /etc/init.d/atopacct
        - require_in:
            - service: atop-service

atopacct:
  service.dead:
    - enable: false
    - onlyif:
        - stat /lib/systemd/system/atopacct.service

pin-atop-from-mirror:
  file.managed:
    - name: /etc/apt/preferences.d/41pin-atop
{%     if salt.pillar.get('data:dist:bionic:secure') %}
    - contents: |
        Package: atop
        Pin: release o=yandex-cloud-upstream-bionic-secure
        Pin-Priority: 1000
{%     else %}
    - contents: |
        Package: atop
        Pin: origin "mirror.yandex.ru"
        Pin-Priority: 1000
{%     endif %}
    - user: root
    - group: root
    - permissions: 644

purge-yandex-atop:
  pkg.purged:
    - name: atop
    - version: 1.27-10

atop-package:
  pkg.installed:
    - pkgs:
      - atop: 2.3.0-1
      - acct
    - require:
      - file: pin-atop-from-mirror
      - pkg: purge-yandex-atop

/etc/default/atop:
  file.managed:
    - source: salt://components/atop/conf/atop-default
    - user: root
    - group: root
    - permissions: 644
    - template: jinja

atop-service:
  service.running:
    - enable: True
    - name: atop
    - watch:
      - file: /etc/default/atop
      - pkg: atop-package

/etc/cron.d/wd-atop:
  file.managed:
    - source: salt://components/atop/conf/atop-watchdog
    - user: root
    - group: root
    - permissions: 644
{% else %}
atop-package:
  pkg.purged:
    - name: atop

pin-atop-from-mirror:
  file.absent:
    - name: /etc/apt/preferences.d/41pin-atop

/etc/default/atop:
  file.absent

/etc/cron.d/wd-atop:
  file.absent

acct-package:
  pkg.purged:
    - name: acct
    - require:
      - pkg: atop-package
{% endif %}
