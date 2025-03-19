{% if salt['pillar.get']('minion:managed', True) %} # default value is True for backward compatibility
base:
  '*':
    - templates.salt-minion
{% if salt['pillar.get']('salt_status') == 'master' %}
    - templates.salt-master
    - templates.salt-rsync
{% endif %}
{% if salt['pillar.get']('dynamic_roots', False) %}
    - templates.dynamic-roots
{% endif %}
{% endif %}
