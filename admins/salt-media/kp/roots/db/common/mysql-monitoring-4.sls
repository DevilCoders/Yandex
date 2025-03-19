{% set env = grains['yandex-environment'] %}

/etc/mysql-configurator/monitoring/metrics-kp.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/metrics-kp.yaml
    - mode: 644
    - makedirs: True
    - watch_in:
      - cmd: restart_monitoring4

/etc/mysql-configurator/monitoring/main.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/monitoring-main.yaml
    - mode: 644
    - makedirs: True
    - watch_in:
      - cmd: restart_monitoring4

/etc/mysql-configurator/replica-watcher/main.yaml:
  yafile.managed:
    - source: salt://{{ slspath }}/files/replica-watcher-main.yaml
    - mode: 644
    - makedirs: True
    - watch_in:
      - cmd: restart_rwatcher4

{% set zk = 'kp-stable-zk' if env in ['production', 'prestable'] else 'kp-test-zk' %}
/etc/mysql-configurator/zookeeper.yaml:
  file.managed:
    - mode: 644
    - makedirs: True
    - watch_in:
      - cmd: restart_rwatcher4
      - cmd: restart_monitoring4
    - contents: |
        hosts_url: https://c.yandex-team.ru/api-cached/groups2hosts/{{ zk }}
        prefix: /mysql-configurator-4
        port: 2181
        timeout: 300

restart_monitoring4:
  cmd.wait:
    - name: ubic restart mysql.monitoring-4

restart_rwatcher4:
  cmd.wait:
    - name: ubic restart mysql.replica-watcher-4
