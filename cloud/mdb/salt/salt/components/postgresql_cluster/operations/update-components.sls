{% from "components/postgres/pg.jinja" import pg with context %}
{% if salt.pillar.get('update-odyssey', False) or salt.pillar.get('update-postgresql', False) %}

all-pg-packages-are-ready:
{% if salt.pillar.get('update-postgresql', False) %}
    test.nop:
        - require:
            - pkg: postgresql{{ pg.version.major }}-server
        - require_in:
            - cmd: postgresql-stop
{% else %}
    test.nop
{% endif %}



include:
    - components.postgres.service
    - components.repositories
{% if salt.pillar.get('update-odyssey', False) %}
    - .update-odyssey
{% endif %}

{% if salt.pillar.get('update-postgresql', False) %}
    - components.monrun2.update-jobs
    - components.monrun2.pg-common
    - components.postgres.users
    - .update-postgresql
{% endif %}

{% endif %}
