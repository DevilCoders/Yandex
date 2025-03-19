{% from "templates/postgresql/map.jinja" import postgres with context %}

{% if postgres.use_upstream_repo %}
include:
  - templates.postgresql.upstream
{% endif %}

install-postgresql-client:
  pkg.installed:
    - name: {{ postgres.pkg_client }}
    - refresh: {{ postgres.use_upstream_repo }}

{% if postgres.pkg_libpq_dev != False %}
install-postgres-libpq-dev:
  pkg.installed:
    - name: {{ postgres.pkg_libpq_dev }}
{% endif %}
