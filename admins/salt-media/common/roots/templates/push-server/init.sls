{% from "templates/push-server/map.jinja" import push_server with context %}

push_server:
  pkg:
    - installed
    - name: {{ push_server.package }}
