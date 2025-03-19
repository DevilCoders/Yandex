include:
{% if salt.grains.get('os') != 'Windows' %}
    - components.common.dbaas
{%   if not salt.dbaas.is_aws() %}
    - components.common.clean-etc-hosts
    - components.common.etc-hosts
{%   endif %}
{%   if salt['pillar.get']('firewall') %}
    - components.firewall.reload
    - components.firewall.force-reload
    - components.firewall.pillar_configs
    - components.firewall.external_access_config
{%     if salt['pillar.get']('data:dbaas:cluster_id', False) %}
    - components.firewall.user_net_config
{%     endif %}
{%   endif %}
{% else %}
    - components.common-windows.firewall
    - components.common-windows.cluster
{% endif %}
