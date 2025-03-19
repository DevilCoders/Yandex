{% from "components/postgres/pg.jinja" import pg with context %}

libgeos-c1v5:
    pkg.installed:
        - version: '3.7.1-1~pgdg18.04+1+yandex0'
        - prereq_in:
            - cmd: repositories-ready
        - require_in:
            - postgis-packages
        - require_in:
            - cmd: all-pg-packages-are-ready

{% if pg.version.major_num >= 1300 %}
{% set postgis_major_version = '3' %}
{% set postgis_pkg_version = '3.1.4+dfsg-3.pgdg18.04+1' %}
{% elif pg.version.major_num == 1200 %}
{% set postgis_major_version = '3' %}
{% set postgis_pkg_version = '3.0.0+dfsg-2~exp1.pgdg18.04+1+yandex0' %}
{% else %}
{% set postgis_major_version = '2.5' %}
{% set postgis_pkg_version = '2.5.2+dfsg-1~exp1.pgdg18.04+1+yandex0' %}
{% endif %}

{# prefer the version from data:versions, if set #}
{% set postgis_major_version = salt['pillar.get']('data:versions:postgis:major_version', postgis_major_version) %}
{% set postgis_pkg_version = salt['pillar.get']('data:versions:postgis:package_version', postgis_pkg_version) %}

{% if pg.version.major_num == 1400 %}
{% set pgrouting_pkg_version = '3.3.1-1' %}
{% elif pg.version.major_num == 1300 %}
{% set pgrouting_pkg_version = '3.0.2-1.pgdg18.04+1+yandex0' %}
{% else %}
{% set pgrouting_pkg_version = '2.6.2-1.pgdg18.04+1+yandex0' %}
{% endif %}

{% set pgrouting_pkg_version = salt['pillar.get']('data:versions:pgrouting:package_version', pgrouting_pkg_version) %}

liblwgeom-2.5-0:
    pkg.installed:
        - version: '2.5.4+dfsg-1.pgdg18.04+1+yandex0'
        - prereq_in:
            - cmd: repositories-ready
        - require_in:
            - cmd: all-pg-packages-are-ready

postgis-packages:
    pkg.installed:
        - pkgs:
{# some hacks for deprecated pg and postigs #}
{% if pg.version.major_num == 1000 %}
            - postgresql-10-postgis-3: '3.1.4+dfsg-3.pgdg18.04+1'
            - postgresql-10-postgis-3-scripts: '3.1.4+dfsg-3.pgdg18.04+1'
{% elif pg.version.major_num == 1100 %}
            - postgresql-11-postgis-3: '3.0.0+dfsg-2~exp1.pgdg18.04+1+yandex0'
            - postgresql-11-postgis-3-scripts: '3.0.0+dfsg-2~exp1.pgdg18.04+1+yandex0'
{% endif %}
            - postgresql-{{ pg.version.major }}-postgis-{{ postgis_major_version }}: {{ postgis_pkg_version }}
            - postgresql-{{ pg.version.major }}-postgis-{{ postgis_major_version }}-scripts: {{ postgis_pkg_version }}
            - postgresql-{{ pg.version.major }}-pgrouting: {{ pgrouting_pkg_version }}
            - postgresql-{{ pg.version.major }}-pgrouting-scripts : {{ pgrouting_pkg_version }}
        - require:
            - pkgrepo: pgdg
            - pkg: postgresql{{ pg.version.major }}-server
        - require_in:
            - cmd: all-pg-packages-are-ready
