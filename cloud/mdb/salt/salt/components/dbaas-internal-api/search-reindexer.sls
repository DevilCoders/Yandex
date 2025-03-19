
balancer-in-etc-hosts:
    host.present:
        - name: {{ salt['dbaas.pillar']('data:internal_api:server_name') }}
        - ip: {{ salt['grains.get']('fqdn_ip6') | tojson }}
        - clean: True

/etc/yandex/mdb-search-reindexer:
    file.directory:
        - makedirs: True
        - mode: 755
        - user: web-api
        - group: root
        - require:
              - user: web-api-user

/etc/yandex/mdb-search-reindexer/mdb-search-reindexer.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer.yaml
        - mode: 600
        - user: web-api
        - group: root
        - require:
              - file: /etc/yandex/mdb-search-reindexer

/etc/yandex/mdb-search-reindexer/zk-flock.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer-zk-flock.json
        - mode: 600
        - user: web-api
        - group: root
        - require:
              - file: /etc/yandex/mdb-search-reindexer

search-reindexer-deps-pkgs:
    pkg.installed:
        - pkgs:
              - yazk-flock

/etc/cron.d/mdb-search-reindexer:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer.cron
        - mode: 644
        - require:
              - pkg: search-reindexer-deps-pkgs

/etc/logrotate.d/mdb-search-reindexer:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer-logrotate.conf
        - mode: 644
        - makedirs: True

/var/log/mdb-search-reindexer:
    file.directory:
        - user: web-api
        - group: web-api
        - require:
              - user: web-api-user
