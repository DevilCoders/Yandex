{% set unit = 'oom-check' %}

{% for file in pillar.get('oom-check-config-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{unit}}{{file}}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
