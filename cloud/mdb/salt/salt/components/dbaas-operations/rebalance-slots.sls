{% set runlist=salt['pillar.get']('data:runlist', []) %}
{% if runlist %}
include:
{% for component in runlist %}
    - {{ component }}.operations.rebalance-slots
{% endfor %}
{% endif %}
