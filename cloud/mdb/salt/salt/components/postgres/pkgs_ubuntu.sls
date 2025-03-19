{% from "components/postgres/pg.jinja" import pg with context %}
{% set osrelease = salt['grains.get']('osrelease') %}
{% set env = salt['pillar.get']('yandex:environment', 'prod') %}

include:
    - .postgres-pkg
    - components.repositories
    - components.repositories.apt.pgdg
    - components.repositories.apt.mdb-bionic.stable
{% if salt['pillar.get']('data:use_postgis', False) %}
    - components.postgres.postgis
{% endif %}

postgresql-common-pkg:
    pkg.installed:
        - pkgs:
            - postgresql-common: '242-3-pgdg18.04+1+yandex220'
            - postgresql-client-common: '242-3-pgdg18.04+1+yandex220'
        - prereq_in:
            - cmd: repositories-ready
    file.managed:
        - name: /etc/postgresql-common/createcluster.conf
        - require:
            - pkg: postgresql-common-pkg
        - require_in:
            - pkg: postgresql{{ pg.version.major }}-server
        - template: jinja
        - source: salt://{{ slspath + '/conf/createcluster.conf' }}

pgtop:
    pkg.latest:
        - refresh: False
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num < 1300 %}
{%    set kcache_pkg = 'postgresql-' + pg.version.major + '-kcache' %}
{% else %}
{%    set kcache_pkg = 'postgresql-' + pg.version.major + '-pg-stat-kcache' %}
{% endif %}

{%  if salt['pillar.get']('data:kcache:version') %}
{%      set kcache_version = salt['pillar.get']('data:kcache:version') %}
{%  else %}
{%      if pg.version.major_num < 1200 %}
{%          set kcache_version = '96-4f0f1ff' %}
{%      elif pg.version.major_num == 1300 %}
{%          set kcache_version = '2.1.3-1.pgdg18.04+1+yandex0' %}
{%      elif pg.version.major_num == 1400 %}
{%          set kcache_version = '2.2.0-2.pgdg18.04+1' %}
{%      else %}
{%          set kcache_version = '97-123076e' %}
{%      endif %}
{%  endif %}

{# prefer the kcache version from data:versions, if set #}
{% set kcache_version = salt['pillar.get']('data:versions:kcache:package_version', kcache_version) %}

{% if salt['pillar.get']('data:diagnostic_tools', True) %}
postgresql-{{ pg.version.major }}-kcache:
    pkg.installed:
        - name: {{ kcache_pkg }}
        - version: {{ kcache_version }}
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% if pg.version.major_num < 1200 %}
{% set partman_version = '4.0.0-1.pgdg' ~ osrelease ~ '+2+yandex0' %}
{% elif pg.version.major_num == 1300 %}
{% set partman_version = '4.4.0-3.pgdg18.04+1+yandex0' %}
{% elif pg.version.major_num == 1400 %}
{% set partman_version = '4.6.0-1.pgdg' ~ osrelease ~ '+1' %}
{% else %}
{% set partman_version = '4.2.0-1.pgdg' ~ osrelease ~ '+1+yandex0' %}
{% endif %}
{% set partman_version = salt['pillar.get']('data:versions:partman:package_version', partman_version) %}

postgresql-{{ pg.version.major }}-partman:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-partman
        - version: {{ partman_version }}
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num == 1400 %}
{% set wal2json_version = '2.4-2.pgdg18.04+1' %}
{% elif pg.version.major_num == 1300 %}
{% set wal2json_version = '2.3-2.pgdg18.04+1' %}
{% else %}
{% set wal2json_version = '92-a916352' %}
{% endif %}
{% set wal2json_version = salt['pillar.get']('data:versions:wal2json:package_version', wal2json_version) %}

postgresql-{{ pg.version.major }}-wal2json:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-wal2json
        - version: {{ wal2json_version }}
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num == 1400 %}
{% set repack_version = '1.4.7-1-4211335' %}
{% else %}
{% set repack_version = '1.4.6-1' %}
{% endif %}
{% set repack_version = salt['pillar.get']('data:versions:repack:package_version', repack_version) %}

postgresql-{{ pg.version.major }}-repack:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-repack
        - version: '{{ repack_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num == 1000 %}
{% set lwaldump_version = '1.0-13-d7b5240' %}
{% elif pg.version.major_num == 1100 %}
{% set lwaldump_version = '1.0-7-a01e9a4' %}
{% elif pg.version.major_num == 1200 %}
{% set lwaldump_version = '1.0-8-e671226' %}
{% elif pg.version.major_num == 1300 %}
{% set lwaldump_version = '1.0-13-cdcecb4' %}
{% elif pg.version.major_num == 1400 %}
{% set lwaldump_version = '1.0-14-e35d2fa' %}
{% endif %}
{% set lwaldump_version = salt['pillar.get']('data:versions:lwaldump:package_version', lwaldump_version) %}

{% if salt['pillar.get']('data:use_pgsync', True) %}
lwaldump-{{ pg.version.major }}:
    pkg.installed:
        - name: lwaldump-{{ pg.version.major }}
        - version: '{{ lwaldump_version }}'
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% if pg.version.major_num == 1000 %}
{% set heapcheck_version = '34-d95f927' %}
{% elif pg.version.major_num == 1100 %}
{% set heapcheck_version = '36-0d6578c' %}
{% elif pg.version.major_num == 1200 %}
{% set heapcheck_version = '35-cc765b2' %}
{% elif pg.version.major_num == 1300 %}
{% set heapcheck_version = '37-4858f4d' %}
{% endif %}
{% set heapcheck_version = salt['pillar.get']('data:versions:heapcheck:package_version', heapcheck_version) %}

{% if pg.version.major_num < 1400 %}
postgresql-{{ pg.version.major }}-heapcheck:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-heapcheck
        - version: '{{ heapcheck_version }}'
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% if pg.version.major_num == 1400 %}
{% set logerrors_version = '50-0eabc82' %}
{% else %}
{% set logerrors_version = '40-3c55887' %}
{% endif %}
{% set logerrors_version = salt['pillar.get']('data:versions:logerrors:package_version', logerrors_version) %}

postgresql-{{ pg.version.major }}-logerrors:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-logerrors
        - version: '{{ logerrors_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num >= 1100 and pg.version.major_num < 1400 %}
{% set clickhouse_fdw_version = '1.3-299-aac29ce-yandex' %}
{% set clickhouse_fdw_version = salt['pillar.get']('data:versions:clickhouse_fdw:package_version', clickhouse_fdw_version) %}

postgresql-{{ pg.version.major }}-clickhouse-fdw:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-clickhouse-fdw
        - version: '{{ clickhouse_fdw_version }}'
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% if pg.version.major_num == 1000 %}
{% set oracle_fdw_version = '2.541-f78cb3d' %}
{% elif pg.version.major_num == 1100 %}
{% set oracle_fdw_version = '2.541-8f19219' %}
{% elif pg.version.major_num == 1200 %}
{% set oracle_fdw_version = '2.546-96a42ba' %}
{% elif pg.version.major_num == 1300 %}
{% set oracle_fdw_version = '2.541-674da6f' %}
{% elif pg.version.major_num == 1400 %}
{% set oracle_fdw_version = '2.542-e1059f0' %}
{% endif %}
{% set oracle_fdw_version = salt['pillar.get']('data:versions:oracle_fdw:package_version', oracle_fdw_version) %}

postgresql-{{ pg.version.major }}-oracle-fdw:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-oracle-fdw
        - version: '{{ oracle_fdw_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% set orafce_version = '3.18-546-b5204da-yandex' %}
{% set orafce_version = salt['pillar.get']('data:versions:orafce:package_version', orafce_version) %}

postgresql-{{ pg.version.major }}-orafce:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-orafce
        - version: '{{ orafce_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num == 1000 %}
{% set rum_version = '10.6-d3b9ede' %}
{% elif pg.version.major_num == 1100 %}
{% set rum_version = '11.6-d3b9ede' %}
{% elif pg.version.major_num == 1200 %}
{% set rum_version = '12.6-d3b9ede' %}
{% elif pg.version.major_num == 1300 %}
{% set rum_version = '13.6-d3b9ede' %}
{% elif pg.version.major_num == 1400 %}
{% set rum_version = '1.3.9-ec1c705' %}
{% endif %}
{% set rum_version = salt['pillar.get']('data:versions:rum:package_version', rum_version) %}

postgresql-{{ pg.version.major }}-rum:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-rum
        - version: '{{ rum_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% set libreoffice_dictionaries_version= salt['pillar.get']('data:versions:libreoffice-dictionaries:package_version', '1:6.0.3-3') %}
libreoffice-dictionaries:
     pkg.installed:
        - prereq_in:
            - cmd: repositories-ready
        - pkgs:
          - hunspell-cs: '{{ libreoffice_dictionaries_version }}'
          - hunspell-da: '{{ libreoffice_dictionaries_version }}'
          - hunspell-de-de-frami: '{{ libreoffice_dictionaries_version }}'
          - hunspell-en-gb: '{{ libreoffice_dictionaries_version }}'
          - hunspell-es: '{{ libreoffice_dictionaries_version }}'
          - hunspell-it: '{{ libreoffice_dictionaries_version }}'
          - hunspell-pl: '{{ libreoffice_dictionaries_version }}'
          - hunspell-ru: '{{ libreoffice_dictionaries_version }}'
          - hunspell-uk: '{{ libreoffice_dictionaries_version }}'

{% set plv8_pkg_version = '2.5-2ed1271' %}
{% if pg.version.major_num == 1400 %}
{% set plv8_pkg_version = '3.0.0-cc39a3f' %}
{% endif %}
{% set plv8_version = salt['pillar.get']('data:versions:plv8:package_version', plv8_pkg_version) %}
postgresql-{{ pg.version.major }}-plv8:
     pkg.installed:
        - name: postgresql-{{ pg.version.major }}-plv8
        - version: '{{ plv8_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% set hypopg_version = salt['pillar.get']('data:versions:hypopg:package_version', '1.3.1-258-57d711b') %}
postgresql-{{ pg.version.major }}-hypopg:
     pkg.installed:
        - name: postgresql-{{ pg.version.major }}-hypopg
        - version: '{{ hypopg_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% set pgvector_version = salt['pillar.get']('data:versions:pgvector:package_version', '200-0e6e9f3') %}
postgresql-{{ pg.version.major }}-pgvector:
     pkg.installed:
        - name: pgvector-{{ pg.version.major }}
        - version: '{{ pgvector_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% set pg_qualstats_version = salt['pillar.get']('data:versions:pg_qualstats:package_version', '2.0.3-236-4d00e50') %}
pg-qualstats-{{ pg.version.major }}:
     pkg.installed:
        - name: pg-qualstats-{{ pg.version.major }}
        - version: '{{ pg_qualstats_version }}'
        - prereq_in:
            - cmd: repositories-ready

{% if pg.version.major_num >= 1100 %}
{% set timescaledb_2_3_1_package_version = '2791-a42284fb' %}
{% set timescaledb_2_4_2_package_version = '2891-a83e2b0f' %}
{% set timescaledb_2_5_2_package_version = '3060-6794b140' %}
{% set timescaledb_2_6_1_package_version = '3198-9ae47c68' %}
timescaledb-packages:
    pkg.installed:
        - pkgs:
{% if pg.version.major_num == 1100 %}
            - postgresql-11-timescaledb-2.3.1: {{ timescaledb_2_3_1_package_version }}
{% elif pg.version.major_num == 1200 %}
            - postgresql-12-timescaledb-2.3.1: {{ timescaledb_2_3_1_package_version }}
            - postgresql-12-timescaledb-2.4.2: {{ timescaledb_2_4_2_package_version }}
{% elif pg.version.major_num == 1300 %}
            - postgresql-13-timescaledb-2.4.2: {{ timescaledb_2_4_2_package_version }}
            - postgresql-13-timescaledb-2.5.2: {{ timescaledb_2_5_2_package_version }}
{% elif pg.version.major_num == 1400 %}
            - postgresql-14-timescaledb-2.5.2: {{ timescaledb_2_5_2_package_version }}
            - postgresql-14-timescaledb-2.6.1: {{ timescaledb_2_6_1_package_version }}
{% endif %}
{% endif %}

{% if pg.version.major_num == 1000 %}
{% set pg_hint_plan_version = '1.3.3-608-06f70ae' %}
{% elif pg.version.major_num == 1100 %}
{% set pg_hint_plan_version = '1.3.4-613-7e0df54' %}
{% elif pg.version.major_num == 1200 %}
{% set pg_hint_plan_version = '1.3.5-630-3f8dc11' %}
{% elif pg.version.major_num == 1300 %}
{% set pg_hint_plan_version = '1.3.7-646-d99fbc9' %}
{% else %}
{% set pg_hint_plan_version = '1.4-662-501fdfc' %}
{% endif %}
{% set pg_hint_plan_version = salt['pillar.get']('data:versions:pghintplan:package_version', pg_hint_plan_version) %}

{% if not pg.use_1c %}
postgresql-{{ pg.version.major }}-pg-hint-plan:
    pkg.installed:
        - name: postgresql-{{ pg.version.major }}-pg-hint-plan
{% if pg_hint_plan_version %}
        - version: '{{ pg_hint_plan_version }}'
{% endif %}
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% set perf_diag_version = '8-150482b' %}

{% if pg.version.major_num < 1400 %}
postgresql-{{ pg.version.major }}-mdb-perf-diag:
    pkg.installed:
        - pkgs:
            - postgresql-{{ pg.version.major }}-mdb-perf-diag: {{ perf_diag_version }}
        - prereq_in:
            - cmd: repositories-ready
{% endif %}

{% set pg_cron_version = salt['pillar.get']('data:versions:cron:package_version', '1.4.1-2.pgdg18.04+1') %}
postgresql-{{ pg.version.major }}-cron:
    pkg.installed:
        - pkgs:
            - postgresql-{{ pg.version.major }}-cron: {{ pg_cron_version }}
        - prereq_in:
            - cmd: repositories-ready

install-pg-additional-packages:
    pkg.installed:
        - pkgs:
            - python3-paramiko
            - python3-tenacity
        - require:
            - cmd: repositories-ready

all-pg-packages-are-ready:
    test.nop:
       - require:
            - postgresql{{ pg.version.major }}-server
            - pgtop
            - postgresql-{{ pg.version.major }}-partman
            - postgresql-{{ pg.version.major }}-wal2json
            - postgresql-{{ pg.version.major }}-repack
            - postgresql-{{ pg.version.major }}-logerrors
            - postgresql-{{ pg.version.major }}-rum
            - libreoffice-dictionaries
            - postgresql-{{ pg.version.major }}-plv8
            - postgresql-{{ pg.version.major }}-hypopg
            - postgresql-{{ pg.version.major }}-pgvector
            - pg-qualstats-{{ pg.version.major }}
            - postgresql-{{ pg.version.major }}-oracle-fdw
            - postgresql-{{ pg.version.major }}-orafce
            - postgresql-{{ pg.version.major }}-cron
{% if salt['pillar.get']('data:diagnostic_tools', True) %}
            - postgresql-{{ pg.version.major }}-kcache
{% endif %}
{% if pg.version.major_num < 1400 %}
            - postgresql-{{ pg.version.major }}-heapcheck
{% endif %}
{% if pg.version.major_num < 1400 %}
            - postgresql-{{ pg.version.major }}-mdb-perf-diag
{% endif %}
{% if not pg.use_1c %}
            - postgresql-{{ pg.version.major }}-pg-hint-plan
{% endif %}
{% if pg.version.major_num >= 1100 %}
            - timescaledb-packages
{% endif %}
{% if salt['pillar.get']('data:use_pgsync', True) %}
            - lwaldump-{{ pg.version.major }}
{% endif %}
{% if pg.version.major_num >= 1100 and pg.version.major_num < 1400 %}
            - postgresql-{{ pg.version.major }}-clickhouse-fdw
{% endif %}
