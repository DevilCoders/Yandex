include:
{% if not salt['pillar.get']('data:kafka:has_zk_subcluster') and not salt['pillar.get']('data:running_on_template_machine', False) %}
    - components.zk
{% endif %}
    - components.kafka
