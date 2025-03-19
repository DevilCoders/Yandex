{% set unit = 'packages' %}
{% set cluster=pillar.get("cluster") %}

{% if pillar.get('packages-list') %}
{% for file in pillar.get('packages-list') %}
{{file}}:
  pkg:
   - installed
{% endfor %}
{% endif %}

{% if pillar.get('versioned-packages-list') %}
{% for pkg in pillar.get('versioned-packages-list') %}
{% for name, version in pkg.items() %}
{{ name }}:
  pkg:
   - installed
   - version: {{ version }}
{% endfor %}
{% endfor %}
{% endif %}

{% if pillar.get('packages-files') %}
{% for file in pillar.get('packages-files') %}
{{file}}:
  yafile.managed:
    - source: salt://files/{{ cluster }}{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}
{% endif %}
