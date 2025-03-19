{% from "templates/proftpd/map.jinja" import proftpd with context %}

proftpd:
  pkg:
    - installed
    - name: {{ proftpd.package }}
