{% from slspath + "/map.jinja" import rsyncd with context %}

rsync_server:
  pkg.installed:
    - pkgs:
    {%- for pkg in rsyncd.packages %}
      - {{ pkg }}
    {%- endfor %}
  service.running:
    - name: {{ rsyncd.service }}
    - enable: True
