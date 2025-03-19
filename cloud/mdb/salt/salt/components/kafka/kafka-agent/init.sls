kafka-agent-pkgs:
    pkg.installed:
        - pkgs:
            - mdb-kafka-agent: '1.9689834'
        - require:
            - cmd: repositories-ready

# Overwrite kafka-agent binary in infratest environment
{% if salt['cp.hash_file']('salt://' + slspath + '/conf/kafka-agent', saltenv) %}
/opt/yandex/kafka-agent/bin/kafka-agent:
    file.managed:
      - source: salt://{{ slspath + '/conf/kafka-agent' }}
      - user: root
      - group: root
      - mode: 755
      - require:
          - pkg: kafka-agent-pkgs
      - require_in:
          - service: kafka-agent-service
      - watch_in:
          - service: kafka-agent-service
{% endif %}

kafka-agent-user:
    user.present:
        - fullname: Kafka Agent user
        - name: kafka-agent
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

/etc/yandex/kafka-agent:
    file.directory:
        - user: kafka-agent
        - mode: 755
        - makedirs: True
        - require:
            - user: kafka-agent-user

/etc/yandex/kafka-agent/kafka-agent.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/kafka-agent.yaml' }}
        - mode: '0640'
        - user: kafka-agent
        - makedirs: True
        - require:
            - user: kafka-agent-user
            - file: /etc/yandex/kafka-agent

/var/log/kafka-agent:
    file.directory:
        - user: kafka-agent
        - group: kafka-agent
        - dir_mode: 751
        - recurse:
            - user
            - group
        - makedirs: True
        - require:
            - user: kafka-agent-user

/etc/logrotate.d/kafka-agent:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

/lib/systemd/system/kafka-agent.service:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/kafka-agent.service
        - mode: 644
        - require:
            - pkg: kafka-agent-pkgs
            - user: kafka-agent-user
        - onchanges_in:
            - module: systemd-reload

kafka-agent-service:
    service.running:
        - name: kafka-agent
        - enable: True
        - require:
            - file: /lib/systemd/system/kafka-agent.service
            - file: /var/log/kafka-agent
        - watch:
            - pkg: kafka-agent-pkgs
            - file: /lib/systemd/system/kafka-agent.service
            - file: /etc/yandex/kafka-agent/kafka-agent.yaml
            - file: /var/log/kafka-agent
