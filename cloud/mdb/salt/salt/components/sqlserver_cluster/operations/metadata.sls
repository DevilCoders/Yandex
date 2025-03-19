include:
    - components.sqlserver.firewall
{% if not salt['pillar.get']('skip-billing', False) %}
    - components.dbaas-billing-windows.config
{% endif %}
