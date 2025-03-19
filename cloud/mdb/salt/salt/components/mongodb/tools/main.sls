{% set mongodb = salt.slsutil.renderer('salt://components/mongodb/defaults.py?saltenv=' ~ saltenv) %}

{% set srv = 'mongod' %}
{% set osrelease = salt.grains.get('osrelease') %}

{% set confdir = salt.mdb_mongodb_helpers.mdb_mongo_tools_confdir() %}
{% set log = '/var/log/mongodb/mdb-mongo-tools.log' %}

include:
    - .configs


/etc/cron.d/mdb-mongod-resetup:
    file.managed:
        - contents: |
             PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
             */5 * * * * root zk-flock -c {{confdir}}/zk-flock-resetup.json lock "/usr/bin/mdb-mongod-resetup " >> {{log}} 2>&1
        - mode: 644
        - require:
            - file: {{confdir}}/mdb-mongo-tools.conf
            - file: {{confdir}}/zk-flock-resetup.json

mongod-resetup-zk-root:
    zookeeper.present:
        - name: {{ mongodb.zk_resetup_root }}
        - value: ''
        - makepath: True
        - hosts: {{ mongodb.zk_hosts }}
        - require:
            - pkg: mdb-mongo-tools-packages

/usr/local/yandex/ensure_no_primary.sh:
    file.managed:
        - source: salt://{{ slspath }}/scripts/ensure_no_primary.sh
        - mode: 755
        - makedirs: True
        - require:
            - pkg: mdb-mongo-tools-packages
            - file: {{confdir}}/mdb-mongo-tools.conf
            - file: {{confdir}}/zk-flock-resetup.json

/usr/local/bin/mdb-sample-backtraces.sh:
    file.managed:
        - source: salt://{{ slspath }}/scripts/mdb-sample-backtraces.sh
        - mode: 755
        - makedirs: True

mdb-mongo-tools-ready:
    test.nop:
        - require:
            - pkg: mdb-mongo-tools-packages
            - file: {{confdir}}/mdb-mongo-tools.conf
            - file: {{confdir}}/zk-flock-resetup.json


extend:

  {{confdir}}/mdb-mongo-tools.conf:
    file.managed:
      - require:
        - pkg: mdb-mongo-tools-packages


  {{confdir}}/zk-flock-resetup.json:
    file.managed:
      - require:
        - pkg: mdb-mongo-tools-packages
        - zookeeper: mongod-resetup-zk-root
