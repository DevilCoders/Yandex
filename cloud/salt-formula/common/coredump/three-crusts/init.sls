/var/crashes:
  file.directory:
    - makedirs: True
    - mode: '0777'

kernel.core_pattern:
  sysctl.present:
    - value: /var/crashes/core.%E.%p.%s.%t
    - config: /etc/sysctl.d/90-coredumpctl.conf
    - require:
      - file: /var/crashes

systemd-coredump-disable:
  service.dead:
    - name: systemd-coredump.socket
    - enable: False
    - require:
      - sysctl: kernel.core_pattern

no-yandex-search-coredump-watcher:
  pkg.removed:
    - pkgs:
      - yandex-search-coredump-watcher

/usr/bin/coredump_sender.py:
  file.managed:
    - source: salt://{{ slspath }}/files/coredump_sender.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: true
    - require:
      - yc_pkg: pkg-deps

flock -n /tmp/CORESENDER /usr/bin/coredump_sender.py:
  cron.present:
    - user: root
    - minute: '*/10'
    - require:
      - file: /usr/bin/coredump_sender.py

pkg-deps:
  yc_pkg.installed:
    - pkgs:
      - python3-requests
      - gdb
      - liblz4-tool

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}

include:
  - nginx

{% set nginx_configs = ['coredump-proxy.conf'] %}
{% include 'nginx/install_configs.sls' %}
