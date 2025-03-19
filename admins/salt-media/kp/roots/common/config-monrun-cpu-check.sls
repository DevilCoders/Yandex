{% set yaenv = grains['yandex-environment'] %}

{% if yaenv == 'testing' %}
config-monrun-cpu-check:
  file.managed:
    - makedirs: True
    - name: /etc/config-monrun-cpu-check/config.yml
    - source: salt://{{ slspath }}/files/etc/config-monrun-cpu-check/config.yml
{% endif %}
