{% from "templates/push-server/map.jinja" import push_server with context %}

push_server_monrun:
  pkg:
    - installed
    - name: {{ push_server.monpackage }}

regenerate-monrun-tasks:
  cmd:
    - wait
    - watch:
      - pkg: push_server_monrun
