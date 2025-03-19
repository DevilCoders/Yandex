{% set runlist=salt['pillar.get']('data:runlist', []) %}

{% if runlist %}
include:
{%     if salt['pillar.get']('run-highstate', False) %}
    - components.{{ salt.dbaas.common_component() }}
    - components.runner
{%     else %}
{%         for component in runlist %}
    - {{ component }}.operations.service
{%         endfor %}
{%         if salt['pillar.get']('include-metadata', False) %}
    - .metadata
{%         endif %}
{%         if salt.pillar.get('data:database_slice:enable', False) and not salt['pillar.get']('run-highstate', False) and salt.dbaas.common_component() == 'common' %}
    - components.common.database-slice
{%         endif %}
{%     endif %}
{% endif %}
