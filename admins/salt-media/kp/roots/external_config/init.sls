{% set is_development = grains["yandex-environment"] in ["development"] %}

/etc/yandex/external-config.conf:
  file.managed:
    - user: root
    - group: root
{% if is_development %}
    - mode: 0644
{% else %}
    - mode: 0600
{% endif %}
    - makedirs: True
    - contents: {{ salt['pillar.get']('external_config:content') | json }}
