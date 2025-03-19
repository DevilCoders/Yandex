{% from "components/postgres/pg.jinja" import pg with context %}

{% set debug_suffix = 'dbgsym' %}

postgresql-old-versions-purge:
    pkg.purged:
        - pkgs:
{% for old_version in ['10', '11', '12'] %}
{%     if old_version != pg.version.major %}
            - postgresql-{{ old_version }}
            - postgresql-client-{{ old_version }}
            - postgresql-{{ old_version }}-{{ debug_suffix }}
{%     endif %}
{% endfor %}
        - require:
            - pkg: postgresql{{ pg.version.major }}-server
