{% set api_adapter = pillar.get('api-adapter') %}
{% if api_adapter.get('push_client', {}).get('enabled', False) %}

push-client:
  yc_pkg.installed:
    - name: yandex-push-client
  service.running:
    - name: statbox-push-client
    - enable: True
    - require:
      - yc_pkg: push-client
    - watch:
      - file: /var/spool/push-client
      - file: /etc/yandex/statbox-push-client/push-client.yaml

/etc/yandex/statbox-push-client/push-client.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/push-client.yaml
    - template: jinja
    - user: statbox
    - group: statbox
    - require:
      - yc_pkg: push-client

/etc/logrotate.d/statbox-push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/statbox-push-client.logrotate
    - user: root
    - group: root
    - require:
      - yc_pkg: push-client

/var/spool/push-client:
  file.directory:
    - user: statbox
    - group: statbox
    - require:
      - yc_pkg: push-client

/var/log/statbox:
  file.directory:
    - user: statbox
    - group: statbox
    - require:
      - yc_pkg: push-client


{% endif %}
