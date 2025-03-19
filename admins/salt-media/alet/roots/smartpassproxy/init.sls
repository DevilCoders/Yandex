common_packages:
  pkg.installed:
    - pkgs:
      - yandex-media-common-configinterfacesmock
      - config-caching-dns
      - config-yabs-ntp
      - yandex-timetail
      - config-monitoring-common
      - config-juggler-client-media
      - config-monrun-monitoring-alive
      - config-monrun-reboot-count
      - config-monrun-drop-check
      - yandex-media-common-oom-check
      - yandex-coredump-monitoring
      - yandex-search-ip-tunnel

/etc/network/interfaces:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: source /etc/network/interfaces.d/*.cfg

/etc/logrotate.d/supervisor:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{ slspath }}/files/etc/logrotate.d/supervisor

supervisor:
  pkg.installed

socat:
  pkg.installed

/app_start.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 0750
    - makedirs: True
    - source: salt://{{ slspath }}/files/app_start.sh
    - template: jinja

/iptunnel-key:
  file.managed:
    - user: root
    - group: root
    - mode: 0750
    - makedirs: True
    - contents: {{ salt['pillar.get']('rt_token') }}

/usr/bin/tun4.py:
  file.managed:
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - source: salt://{{ slspath }}/files/usr/bin/tun4.py

/etc/supervisor/conf.d/application.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{ slspath }}/files/etc/supervisor/conf.d/application.conf

supervisor_service:
  service.running:
    - name: supervisor
    - enable: True
    - reload: True
    - watch:
      - pkg: supervisor
      - file: /etc/supervisor/conf.d/application.conf
      - file: /app_start.sh
