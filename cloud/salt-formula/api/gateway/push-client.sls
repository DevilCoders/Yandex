{%- from "api/gateway/map.jinja" import api_gateway with context %}

{% if api_gateway.get('push_client', {}).get('enabled', False) %}

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
      - file: /usr/local/bin/envoy_access_log_to_json.py

/etc/yandex/statbox-push-client/push-client.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/stats/push-client.yaml
    - template: jinja
    - user: statbox
    - group: statbox
    - require:
      - yc_pkg: push-client

/etc/logrotate.d/statbox-push-client:
  file.managed:
    - source: salt://{{ slspath }}/files/stats/statbox-push-client.logrotate
    - user: root
    - group: root
    - require:
      - yc_pkg: push-client

/usr/local/bin/envoy_access_log_to_json.py:
  file.managed:
    - source: salt://{{ slspath }}/files/stats/envoy_access_log_to_json.py
    - mode: 755
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
