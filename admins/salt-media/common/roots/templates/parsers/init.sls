{% set unit = 'parsers' %}

{% for file in pillar.get('parsers-bin') %}
/usr/local/bin/{{file}}:
  yafile.managed:
    - source: salt://templates/parsers/{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
