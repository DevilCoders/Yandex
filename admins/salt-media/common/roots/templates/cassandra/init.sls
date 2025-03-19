{% if "cassandra" in pillar %}
include:
  - .backup
  - .check
  - .report
  - .grants
  - .configs
  - .logrotate
  {% if salt["pillar.get"]("cassandra:stock") %}
  - .repo
  {%- endif %}
  - .pkgs
cassandra_app:
  service.running:
    - name: cassandra
    - enable: True
    - reload: True
    - watch:
      - pkg: cassandra_packages
{% else %}
  {{ Cassandra_pillar_not_DEFINED__my_be_memory_leaks_on_salt_master }}
{% endif %}
