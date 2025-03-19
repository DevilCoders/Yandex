{% from "templates/postgresql/map.jinja" import postgres with context %}

graphite-scripts:
  file.recurse:
    - name: /usr/lib/yandex-graphite-checks/enabled/
    - file_mode: '0755'
    - source: salt://templates/postgresql/files/graphite_scripts
    - include_empty: True
  pkg.installed:
    - name: yandex-graphite-checks-system
    - refresh: True
