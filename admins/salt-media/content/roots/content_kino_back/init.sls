{% set yaenv = grains['yandex-environment'] %}

include:
  - {{ slspath }}.monrun
  - common.graphite_to_solomon

{% if yaenv in ['testing'] %}
/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        warning: 85
        critical: 90
{% endif %}
