/etc/yandex/statbox-push-client/push-client.yaml:
  file.managed:
    - source: salt://configs/push-client/push-client.yaml

/lib/systemd/system/statbox-push-client-watcher.service:
  file.managed:
    - source: salt://services/statbox-push-client-watcher.service

/lib/systemd/system/statbox-push-client-watcher.path:
  file.managed:
    - source: salt://services/statbox-push-client-watcher.path

/var/spool/push-client:
  file.directory:
    - makedirs: True
    - user: statbox
    - group: statbox

/var/spool/push-client/log.empty:
  file.touch:
    - makedirs: True

statbox-push-client-service:
  service.running:
    - name: statbox-push-client.service
    - enable: True
    - require:
      - file: /var/log/fluent
      - file: /var/spool/push-client

statbox-push-client-watcher-path:
  service.running:
    - name: statbox-push-client-watcher.path
    - enable: True
    - running: True
