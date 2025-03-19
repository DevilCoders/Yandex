{% set yaenv = grains['yandex-environment'] %}

{% if yaenv in [ 'testing' ] %}
/etc/monitoring/unispace.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/unispace.conf
{% endif %}
