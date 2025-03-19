{#
    This is very special state, that is designed to be called ONLY FROM BOOTSTRAP.
    Bootstrap calls it after provision of svms (when deploying new clusters or adding new svms).
    Based on roles of current hosts this state chooses what states have to be executed on current host.

    TODO: this umbrella state should be renamed.
#}

{% set fqdn = grains['nodename'] %}
{% set roles = grains.cluster_map.hosts[fqdn].roles %}
{% set include_list = [] %}
{% set oct_heads_created = 'oct_conf' in grains.cluster_map.roles or 'oct_head' in grains.cluster_map.roles %}  {# CLOUD-9405 #}

{% if ('head' in roles) or ('seed' in roles and oct_heads_created) %}
    {% do include_list.append('opencontrail.balancer_all_clusters') %}
{% endif %}

{% if ('compute' in roles) or ('oct_conf' in roles) or ('oct_ctrl' in roles) or ('oct_web' in roles) or ('oct_head' in roles) %}
    {% do include_list.append('opencontrail.balancer_current_cluster') %}
{% endif %}

{% if include_list %}
include: {{ include_list | yaml }}
{% endif %}
