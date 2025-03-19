{% set mongodb = salt.slsutil.renderer('salt://' ~ slspath ~ '/defaults.py?saltenv=' ~ saltenv) %}

{% for srv in mongodb.services_deployed if srv != 'mongos' %}

{% set config = mongodb.config.get(srv) %}
{% set host = salt.grains.get('fqdn') %}

{{srv}}-total-req:
    test.nop

{{srv}}-total-done:
    test.nop

/lib/systemd/system/{{config._srv_name}}.service:
    file.managed:
        - template: jinja
        - require:
            - pkg: mongodb-packages
            - file: /usr/local/yandex/mongoctl.py
        - source: salt://{{ slspath }}/conf/init/mongodb.service
        - context:
            srv: {{srv | tojson}}
            mongodb: {{mongodb | tojson}}
        - mode: 644
        - onchanges_in:
            - module: systemd-reload

{{srv}}-init:
    cmd.run:
        - name: 'rm -rf {{ config.storage.dbPath }}/* && touch {{ mongodb.initialized_flag }}'
        - require:
            - pkg: mongodb-packages
            - cmd: disable-mongod-org
            - cmd: shutdown-mongod-org
            - file: {{ mongodb.homedir }}
            - test: {{srv}}-total-req
        - require_in:
            - file: {{ config.storage.dbPath }}
            - test: {{srv}}-total-done
        - unless:
            - ls {{ mongodb.initialized_flag }} || ls {{ config.storage.dbPath }}/admin

{% if srv == 'mongod' %}
include:
    - .mongod-service
    - .mongod-config
    - .mongod-remove-deleted-rs-members

extend:
    mongod-service:
        service.running:
            - require:
                - pkg: mongodb-packages
                - file: /lib/systemd/system/mongodb.service
                - file: /etc/mongodb/mongodb.conf
                - file: {{config.storage.dbPath}}
                - file: /var/log/mongodb
                - cmd: {{srv}}-init
{%     if mongodb.use_auth and mongodb.cluster_auth == 'keyfile' %}
                - file: {{mongodb.config_prefix}}/mongodb.key
{%     endif %}
{%     if mongodb.use_ssl %}
                - cmd: {{mongodb.config_prefix}}/ssl/certkey.pem
{%     endif %}
                - test: {{srv}}-total-req
            - require_in:
                - test: {{srv}}-total-done

        mdb_mongodb.alive:
          - require:
            - test: {{srv}}-total-req
          - require_in:
            - test: {{srv}}-total-done

    /etc/mongodb/mongodb.conf:
        file.managed:
            - require:
                - pkg: mongodb-packages

    mongod-remove-deleted-rs-members:
        mongodb.replset_remove_deleted:
            - require:
                - mdb_mongodb: mongod-service
            - require_in:
                - test: {{srv}}-total-done
{% elif srv == 'mongocfg' %}
include:
    - .mongocfg-service
    - .mongocfg-config
    - .mongocfg-remove-deleted-rs-members

extend:
    mongocfg-service:
        service.running:
            - require:
                - pkg: mongodb-packages
                - file: /lib/systemd/system/mongocfg.service
                - file: /etc/mongodb/mongocfg.conf
                - file: {{config.storage.dbPath}}
                - file: /var/log/mongodb
                - cmd: {{srv}}-init
{%     if mongodb.use_auth and mongodb.cluster_auth == 'keyfile' %}
                - file: {{mongodb.config_prefix}}/mongodb.key
{%     endif %}
{%     if mongodb.use_ssl %}
                - cmd: {{mongodb.config_prefix}}/ssl/certkey.pem
{%     endif %}
                - test: {{srv}}-total-req
            - require_in:
                - test: {{srv}}-total-done
            - watch:
                - file: /etc/mongodb/mongocfg.conf

        mdb_mongodb.alive:
          - require:
            - test: {{srv}}-total-req
          - require_in:
            - test: {{srv}}-total-done

    /etc/mongodb/mongocfg.conf:
        file.managed:
            - require:
                - pkg: mongodb-packages

    mongocfg-remove-deleted-rs-members:
        mongodb.replset_remove_deleted:
            - require:
                - mdb_mongodb: mongocfg-service
            - require_in:
                - test: {{srv}}-total-done
{% endif %}

{% set admin = salt.pillar.get('data:mongodb:users:admin') %}
{% if salt.mdb_mongodb_helpers.is_replica(srv) %}
{%     set master_hostname = salt.mdb_mongodb_helpers.master_hostname() %}
{%     if not master_hostname %}
{%         set master_hostname = salt.mongodb.find_master(
                  hosts=salt.mdb_mongodb_helpers.replset_hosts(srv),
                  port=config.net.port,
                  timeout=salt.pillar.get('mongodb-primary-timeout', 600),
              ) %}
{%     endif %}
{{srv}}-replicaset-member:
    mdb_mongodb.ensure_member_in_replicaset:
      - require:
        - mdb_mongodb: {{srv}}-service
      - require_in:
        - test: {{srv}}-total-done
      - master_hostname: {{ master_hostname }}
      - service: {{srv}}
{% else %}
{{srv}}-wasnt-initiated:
    mongodb.host_wasnt_initiated:
        - require:
          - mdb_mongodb: {{srv}}-service
        - onlyif:
            # Only fire init if configuration is not initialized yet.
            - test $({{config.cli.cmd}} --quiet --norc --eval "rs.status()['code']" | tail -n1) = 94
        - name: {{salt.mdb_mongodb_helpers.replset_hosts(srv) | join(',')}}
        - port: {{config.net.port}}
        - user: admin
        - password: "{{admin['password']}}"
        - authdb: admin

{{srv}}-replicaset-member:
    mongodb.replset_initiate:
        - require:
            - mdb_mongodb: {{srv}}-service
            - mongodb: {{srv}}-wasnt-initiated
        - require_in:
          - test: {{srv}}-total-done
        - onlyif:
            # Only fire init if configuration is not initialized yet.
            # We have to use 'tail -n1' here because of
            # ssl-related warnings are not beeing suppressed by --quiet
            - test $({{config.cli.cmd}} --quiet --norc --eval "rs.status()['code']" | tail -n1) = 94
        - name: localhost
        - port: {{config.net.port}}
        - authdb: admin

{{srv}}-wait-for-primary-state:
    mongodb.rs_host_role_wait:
        - role: "PRIMARY"
        - wait_secs: 20
        - require:
            - {{srv}}-replicaset-member
        - require_in:
          - test: {{srv}}-total-done
        - name: localhost
        - port: {{config.net.port}}
{% endif %}

{% endfor %}
