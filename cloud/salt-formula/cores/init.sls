/etc/three-crusts/config.py:
  file.managed:
    - source: salt://{{ slspath }}/files/config
    - mode: 644
    - user: root
    - group: root
    - template: jinja

/etc/three-crusts/db-pass:
  file.managed:
    - mode: 600
    - user: nobody
    - group: root
    - replace: False

install-packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-three-crusts-server

three-crusts:
  service.running:
    - enable: True
    - require:
      - yc_pkg: install-packages
      - file: /etc/three-crusts/config.py
      - file: /etc/three-crusts/db-pass

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}
