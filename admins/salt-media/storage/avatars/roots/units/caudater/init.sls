{% set cluster = pillar.get('cluster') %}

caudater:
  pkg.installed:
    - pkgs: {{ salt['conductor.package']('caudater') }}
  service.running:
    - enable: True
    - watch:
      - file: /etc/yandex/caudater/config.yaml

/etc/yandex/caudater/config.yaml:
  file.managed:
    - name: /etc/yandex/caudater/config.yaml
    - source: salt://files/{{ cluster }}/etc/yandex/caudater/config.yaml
    - template: jinja
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - follow_symlinks: True
