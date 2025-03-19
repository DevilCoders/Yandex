{% set log_dir = '/var/log/dbaas-billing' %}
{% set shipping_enabled = salt['pillar.get']('data:billing:ship_logs', True) %}

/etc/vector/vector_billing.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector_billing.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

/etc/vector/vector_billing_test.toml:
    file.managed:
        - source: salt://{{ slspath }}/conf/vector_billing_test.toml
        - mode: 640
        - user: vector
        - template: jinja
        - require:
            - pkg: vector-package
        - watch_in:
            - service: vector-service

vector-in-monitor-group:
  group.present:
    - name: monitor
    - addusers:
        - vector
    - system: True
    - require:
        - pkg: vector-package
    - require_in:
        - service: vector-service

extend:
  vector-validate-conf:
    cmd.run:
      - onchanges:
          - file: /etc/vector/vector_billing.toml
          - file: /etc/vector/vector_billing_test.toml
  vector-test-conf:
    cmd.run:
      - onchanges:
          - file: /etc/vector/vector_billing.toml
          - file: /etc/vector/vector_billing_test.toml
