include:
{% if not salt['pillar.get']('data:mdb-search-producer:logbroker:use_yc_lb') %}
    - components.tvmtool
{% endif %}
    - components.monrun2.mdb-search-producer
    - .mdb-metrics

mdb-search-producer-pkgs:
    pkg.installed:
       - pkgs:
            - mdb-search-producer: '1.9268650'

mdb-search-producer-user:
  user.present:
    - fullname: MDB Search Producer system user
    - name: mdb-search-producer
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data


mdb-search-producer-pillar:
    test.check_pillar:
        - present:
              - 'data:mdb-search-producer:sentry:dsn'
              - 'data:mdb-search-producer:instrumentation:port'
              - 'data:mdb-search-producer:logbroker:topic'

/opt/yandex/mdb-search-producer/mdb-search-producer.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-search-producer.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-search-producer-pkgs
            - test: mdb-search-producer-pillar

mdb-search-producer-supervised:
    supervisord.running:
        - name: mdb-search-producer
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-search-producer
        - watch:
            - pkg: mdb-search-producer-pkgs
            - file: /etc/supervisor/conf.d/mdb-search-producer.conf
            - file: /opt/yandex/mdb-search-producer/mdb-search-producer.yaml

/etc/supervisor/conf.d/mdb-search-producer.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: mdb-search-producer-pkgs


/usr/local/yasmagent/CONF/agent.mdbsearchproducer.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/yasm-agent.mdbsearchproducer.conf
        - mode: 644
        - user: monitor
        - group: monitor
        - require:
            - pkg: yasmagent
        - watch_in:
            - service: yasmagent
