{% set yaenv = grains['yandex-environment'] %}

/etc/monitoring/unispace.conf:
  file.prepend:
    - text: "limit=0.95"

/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        warning: 85
        critical: 90
