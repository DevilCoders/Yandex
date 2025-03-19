{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

include:
    - components.mongodb.mongos-sync-databases

disable-disk-watcher:
    cmd.run:
        - name: /usr/local/yandex/mdb-disk-watcher.py --disable

{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set config = mongodb.config.get(srv) %}
unlock-unfreeze-mongodb-{{ srv }}:
    cmd.run:
        - name: /usr/local/yandex/mdb-disk-watcher.py --unlock --unfreeze --force --pid-file {{config.processManagement.pidFilePath}} --flag-file-locked /tmp/mdb-mongo-fsync.{{srv}}.locked --flag-file-unlocked /tmp/mdb-mongo-fsync.{{srv}}.unlocked --free-mb-limit {{salt.mdb_mongodb_helpers.get_mongo_ro_limit(salt.pillar.get('data:dbaas:space_limit'))}}
        - require:
            - cmd: disable-disk-watcher
        - require_in:
            - sync_mongos_databases
{% endfor %}

enable-disk-watcher:
    cmd.run:
        - name: /usr/local/yandex/mdb-disk-watcher.py --enable
        - require:
            - sync_mongos_databases

{% for srv in mongodb.services_deployed if srv != 'mongos' %}
{%   set config = mongodb.config.get(srv) %}
run-cron-disk-watcher-{{ srv }}:
    cmd.run:
        - name: /usr/local/yandex/mdb-disk-watcher.py --pid-file {{config.processManagement.pidFilePath}} --flag-file-locked /tmp/mdb-mongo-fsync.{{srv}}.locked --flag-file-unlocked /tmp/mdb-mongo-fsync.{{srv}}.unlocked --free-mb-limit {{salt.mdb_mongodb_helpers.get_mongo_ro_limit(salt.pillar.get('data:dbaas:space_limit'))}}
        - require:
            - cmd: enable-disk-watcher
{% endfor %}
