{% if not salt['pillar.get']('skip-billing', False) %}
include:
    - components.dbaas-billing-windows.config
{% endif %}

state-placeholder:
    test.nop
