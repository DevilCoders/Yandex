{% from "templates/postgresql/map.jinja" import postgres with context %}

postgresql-python:
  pkg.installed:
    - name: {{ postgres.python}}
