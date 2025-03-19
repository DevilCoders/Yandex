{% set hostname = grains['nodename'] %}
{% set node_roles = grains['cluster_map']['hosts'][hostname]['roles'] %}
{% set roles_to_include = [] %}

{# some magic to include only existing pillars #}

{% for role in node_roles %}
{% for root in opts['pillar_roots'][saltenv] -%}
   {% if salt['file.file_exists']('{0}/roles/{1}.sls'.format(root, role))  %}
      {% do roles_to_include.append(role) %}
   {% endif %}
 {% endfor %}
{% endfor %}

{# some magic to include only existing pillars #}


{% if roles_to_include %}
include:
{% for role in roles_to_include %}
 - roles.{{ role }}
{% endfor %}
{% endif %}
