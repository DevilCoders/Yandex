Infra configs:
  file.managed:
    - makedirs: True
    - names:
      # juggler configs
      - /etc/api/configs/juggler-client/MANIFEST.json:
        - source: salt://infra/juggler-client/MANIFEST.json
      - /etc/api/configs/juggler-client/juggler-client.conf:
        - source: salt://infra/juggler-client/juggler-client.conf
      - /etc/api/configs/juggler-client/platform-http-checks.json:
        - source: salt://infra/juggler-client/platform-http-checks.json
      # fluent configs
      - /etc/fluent/fluent.conf:
        - source: salt://infra/fluent/fluent.conf
      - /etc/fluent/config.d/containers.input.conf:
        - source: salt://infra/fluent/config.d/containers.input.conf
      - /etc/fluent/config.d/monitoring.conf:
        - source: salt://infra/fluent/config.d/monitoring.conf
      - /etc/fluent/config.d/output.conf:
        - source: salt://infra/fluent/config.d/output.conf
      - /etc/fluent/config.d/system.input.conf:
        - source: salt://infra/fluent/config.d/system.input.conf
