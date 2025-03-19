{% set runlist=salt['pillar.get']('data:runlist', []) %}
{% if runlist %}
include:
{% for component in runlist %}
    - {{ component }}.operations.database-modify
{% endfor %}
{% endif %}
