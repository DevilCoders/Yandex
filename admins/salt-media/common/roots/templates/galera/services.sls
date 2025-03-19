{% from slspath + "/map.jinja" import galera with context %}

percona-repo-config:
  pkg.installed

galera_packages:
  pkg.installed:
    - pkgs:
    {%- for pkg in galera.packages %}
      - {{ pkg }}
    {%- endfor %}
    - refresh: True
    - require:
      - pkg: percona-repo-config

  service.running:
    - name: {{ galera.service }}
    - enable: True
