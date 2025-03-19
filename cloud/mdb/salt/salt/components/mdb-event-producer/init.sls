include:
    - components.monrun2.mdb-event-producer

mdb-event-producer-pkgs:
    pkg.installed:
       - pkgs:
            - mdb-event-producer: '1.9268650'

mdb-event-producer-user:
  user.present:
    - fullname: MDB Event Producer system user
    - name: mdb-event-producer
    - createhome: True
    - empty_password: False
    - shell: /bin/false
    - system: True
    - groups:
        - www-data

/opt/yandex/mdb-event-producer/mdb-event-producer.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-event-producer.yaml' }}
        - mode: '0640'
        - user: root
        - group: www-data
        - makedirs: True
        - require:
            - pkg: mdb-event-producer-pkgs

mdb-event-producer-supervised:
    supervisord.running:
        - name: mdb-event-producer
        - update: True
        - require:
            - service: supervisor-service
            - user: mdb-event-producer
        - watch:
            - pkg: mdb-event-producer-pkgs
            - file: /etc/supervisor/conf.d/mdb-event-producer.conf
            - file: /opt/yandex/mdb-event-producer/mdb-event-producer.yaml

/etc/supervisor/conf.d/mdb-event-producer.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/supervisor.conf
        - require:
            - pkg: mdb-event-producer-pkgs
