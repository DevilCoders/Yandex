{% if not salt['pillar.get']('data:start_mysync', True) %}

mysync-service-disable:
    service.disabled:
        - name: mysync

mysync:
    service.dead

/etc/mysync.yaml:
    file.absent

{% else %}

include:
    - .mysync-config

mysync-service-enable:
    service.enabled:
        - name: mysync
        - require:
            - file: /etc/mysync.yaml

/etc/logrotate.d/mysync:
    file.managed:
        - source: salt://components/mysql/conf/mysync.logrotate
        - template: jinja
        - mode: 644

{%- set mysync_ha_args = salt['grains.get']('fqdn') + " --stream-from " %}
{%- if salt['pillar.get']('data:mysql:replication_source', False) %}
{%- set mysync_ha_args = mysync_ha_args + salt['pillar.get']('data:mysql:replication_source') %}
{%- else %}
{%- set mysync_ha_args = mysync_ha_args + '""' %}
{# Forbidden to set priority on non-HA nodes #}
{%- set mysync_ha_args = mysync_ha_args + " --priority " + salt['pillar.get']('data:mysql:priority', 0)|string %}
{%- endif %}

mysync-ha-configure:
    cmd.run:
        - name: mysync host add {{ mysync_ha_args }} --skip-mysql-check
        - unless: mysync host add {{ mysync_ha_args }} --skip-mysql-check --dry-run
        - require:
            - file: /etc/mysync.yaml

{% endif %}
