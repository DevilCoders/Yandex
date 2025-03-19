{% set runlist=salt['pillar.get']('data:runlist', []) %}
{% if runlist %}
include:
    - components.dbaas-operations.metadata-common
{% for component in runlist %}
    - {{ component }}.operations.metadata
{% endfor %}
{% endif %}
