{% from "components/postgres/pg.jinja" import pg with context %}

{% set debug_suffix = 'dbgsym' %}

postgresql{{ pg.version.major }}-server:
    pkg.installed:
        - pkgs:
            - libpq5: {{ pg.version.pkg }}
            - postgresql-{{ pg.version.major }}: {{ pg.version.pkg }}
            - postgresql-client-{{ pg.version.major }}: {{ pg.version.pkg }}
            - postgresql-{{ pg.version.major }}-{{ debug_suffix }}: {{ pg.version.pkg }}
{% if pg.version.major_num == 1400 %}
            - postgresql-{{ pg.version.major }}-smlar: 4-7503eb8
{% else %}
            - postgresql-{{ pg.version.major }}-smlar: 4-bc4cc3b
{% endif %}
            - postgresql-{{ pg.version.major }}-replmon: 11-acb6ce8
            - postgresql-{{ pg.version.major }}-jsquery
            - pg-tm-aux-{{ pg.version.major }}: 32-676ab3d
{% if pg.use_1c %}
            - postgresql-{{ pg.version.major }}-yndx-1c-aux
{% endif %}
        - refresh: True
{# Unexpectedly prereq_in in this place will fail with `max recursion exceeded` error #}
{# This is actually for 2016 minion. After upgrade to 2018 we need to try again #}
        - require:
            - cmd: repositories-ready
{% if salt['pillar.get']('service-restart') %}
        - require_in:
            - cmd: postgresql-stop
{% endif %}
