{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}
{% set srv = 'mongos' %}
{% set config = mongodb.config.get(srv) %}
{% set host = salt.grains.get('fqdn') %}
{% set admin = salt.pillar.get('data:mongodb:users:admin') %}

mongos-total-req:
    test.nop

mongos-total-done:
    test.nop

/lib/systemd/system/{{srv}}.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/init/mongodb.service
        - context:
            srv: {{srv | tojson}}
            mongodb: {{mongodb | tojson}}
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

include:
    - .mongos-service
    - .mongos-config
    - .mongos-add-shards
    - .mongos-delete-shard
    - .mongos-set-balancer-active-window

extend:
    mongos-service:
        service.running:
            - require:
                - test: mongos-total-req
                - pkg: mongodb-packages
                - file: /lib/systemd/system/{{srv}}.service
                - file: /etc/mongodb/mongos.conf
                - file: /var/log/mongodb
{% if mongodb.use_auth and mongodb.cluster_auth == 'keyfile' %}
                - file: {{mongodb.config_prefix}}/mongodb.key
{% endif %}
{% if mongodb.use_ssl %}
                - cmd: {{mongodb.config_prefix}}/ssl/certkey.pem
{% endif %}
            - require_in:
              - test: mongos-total-done
        mdb_mongodb.alive:
          - require:
            - test: mongos-total-req
          - require_in:
            - test: mongos-total-done

    /etc/mongodb/mongos.conf:
        file.managed:
            - require:
                - pkg: mongodb-packages
            - require_in:
              - test: mongos-total-done

    mongos-shard-delete-req:
        test.nop:
            - require:
                - mdb_mongodb: mongos-service
            - require_in:
              - test: mongos-total-done

    mongos-shards-add-req:
        test.nop:
            - require:
                - mdb_mongodb: mongos-service
            - require_in:
              - test: mongos-total-done

    mongos-set-balancer-active-window:
        test.nop:
            - require:
                - mdb_mongodb: mongos-service
            - require_in:
              - test: mongos-total-done
