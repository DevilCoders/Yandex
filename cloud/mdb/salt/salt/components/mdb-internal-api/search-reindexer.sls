/etc/yandex/mdb-search-reindexer:
    file.directory:
        - makedirs: True
        - mode: 755
        - user: mdb-internal-api
        - group: root
        - require:
              - user: mdb-internal-api-user

/etc/yandex/mdb-search-reindexer/mdb-search-reindexer.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer.yaml
        - mode: 600
        - user: mdb-internal-api
        - group: root
        - require:
              - file: /etc/yandex/mdb-search-reindexer

# ohh ... I made a typo
# TODO: replace that link with file.absent
# TODO: remove that file.absent after api release
/etc/yandex/mdb-search-reindexer/mdb-search-reindex.yaml:
    file.symlink:
        - target: /etc/yandex/mdb-search-reindexer/mdb-search-reindexer.yaml
        - user: mdb-internal-api
        - group: root
        - require:
              - file: /etc/yandex/mdb-search-reindexer/mdb-search-reindexer.yaml

/etc/yandex/mdb-search-reindexer/zk-flock.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer-zk-flock.json
        - mode: 600
        - user: mdb-internal-api
        - group: root
        - require:
              - file: /etc/yandex/mdb-search-reindexer

search-reindexer-deps-pkgs:
    pkg.installed:
        - pkgs:
              - yazk-flock

/etc/logrotate.d/mdb-search-reindexer:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer-logrotate.conf
        - mode: 644
        - makedirs: True

/var/log/mdb-search-reindexer:
    file.directory:
        - user: mdb-internal-api
        - group: www-data
        - require:
              - user: mdb-internal-api-user

/lib/systemd/system/mdb-search-reindexer.service:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer.service
        - mode: 644
        - require:
              - pkg: mdb-internal-api-pkgs
              - user: mdb-internal-api-user
        - onchanges_in:
              - module: systemd-reload

# TODO: Remove that section after next go-api release
mdb-search-reindexer-service:
    service.disabled:
        - name: mdb-search-reindexer.service
        - require:
              - pkg: mdb-internal-api-pkgs
              - file: /lib/systemd/system/mdb-search-reindexer.service
              - file: /etc/yandex/mdb-internal-api/mdb-internal-api.yaml
              - file: /etc/yandex/mdb-search-reindexer/mdb-search-reindexer.yaml
              - file: /var/log/mdb-search-reindexer

/lib/systemd/system/mdb-search-reindexer.timer:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-search-reindexer.timer
        - mode: 644
        - require:
              - pkg: mdb-internal-api-pkgs
              - user: mdb-internal-api-user
              - service: mdb-search-reindexer-service
        - onchanges_in:
              - module: systemd-reload

mdb-search-reindexer-timer:
    service.running:
        - name: mdb-search-reindexer.timer
        - enable: True
        - require:
              - file: /lib/systemd/system/mdb-search-reindexer.timer
