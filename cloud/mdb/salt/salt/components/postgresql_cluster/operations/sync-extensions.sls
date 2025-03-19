{% from "components/postgres/pg.jinja" import pg with context %}
include:
    - components.pg-dbs.unmanaged.sync-extensions
