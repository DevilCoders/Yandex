{% if salt['pillar.get']("nginx:lookup:enabled") %}
include:
  - .services
  - .configs
  - .monrun
  - .dirs
{% else %}
nginx_not_enabled:
  cmd.run:
    - name: echo "nginx:lookup:enabled set to False, or memory leaks on salt master!";exit 1
{% endif %}
