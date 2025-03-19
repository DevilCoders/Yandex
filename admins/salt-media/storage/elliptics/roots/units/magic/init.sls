{% set unit = 'magic' %}

{% for file in pillar.get('magic-files') %}
/etc/{{file}}:
  yafile.managed:
    - source: salt://files/magic/{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
