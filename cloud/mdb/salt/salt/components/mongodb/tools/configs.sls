{% set mongodb = salt.slsutil.renderer('salt://components/mongodb/defaults.py?saltenv=' ~ saltenv) %}
{% set confdir = salt.mdb_mongodb_helpers.mdb_mongo_tools_confdir() %}

{{confdir}}:
    file.directory:
        - user: root
        - group: root
        - mode: 755
        - makedirs: True

{{confdir}}/mdb-mongo-tools.conf:
    file.managed:
        - template: jinja
        - source: salt://{{slspath}}/conf/mdb-mongo-tools.conf
        - mode: 600
        - makedirs: True
        - require:
            - file: {{confdir}}


{{confdir}}/zk-flock-resetup.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - defaults:
            zk_hosts: {{mongodb.zk_hosts}}
            zk_root: {{mongodb.zk_resetup_root}}
        - mode: 644
        - makedirs: True
        - require:
            - file: {{confdir}}
