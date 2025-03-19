include:
    - components.sqlserver.sp_configure
    - components.sqlserver.tasks
    - components.common-windows.firewall
    - components.common-windows.cluster
    - components.sqlserver.firewall
    - components.dbaas-billing-windows.config
{% if salt['pillar.get']('service-restart', False) %}
    - components.sqlserver.restart
{% endif %}
