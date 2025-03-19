{% if salt['grains.get']('cluster_map:hosts:%s:kikimr:cluster_id' % grains['nodename']) %}

{%- import "common/kikimr/init.sls" as vars with context %}

{% set hostname = grains['nodename'] %}
{% set kikimr_config = grains['cluster_map']['hosts'][hostname]['kikimr'] %}
{% set cluster_id = kikimr_config['cluster_id'] %}
{% set kikimr_cluster = grains['cluster_map']['kikimr']['clusters'][cluster_id] %}
{% set storage_nodes = kikimr_cluster['storage_nodes'] %}

{% set dynamic_nodes = [] %}
{% for tenant, dynamic_node in kikimr_cluster['dynamic_nodes'].iteritems() %}
    {% if tenant != vars.nbs_database %}
        {% do dynamic_nodes.extend(dynamic_node) %}
    {% endif %}
{% endfor %}

include:
  - .install
{% if vars.environment in ["dev", "hw-ci", "testing"] or vars.base_role == 'cloudvm' %}
  - .viewer_proxy
{% endif %}
{% if hostname in storage_nodes %}
  - .storage
{% endif %}
{% if hostname in dynamic_nodes %}
  - .tenant
{% endif %}
  - z2

{% endif %}
