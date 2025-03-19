push_client:
  clean_push_client_configs: True
  port: 8088
  stats:
    - name: logbroker
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: rt
      logger: { remote: 0 }
      ident: media-jmon
      logs:
        - file: juggler/frontend/access.log
          log_type: juggler-frontend-log
        - file: juggler/api/access.log
          log_type: juggler-api-log
        - file: juggler/banshee/banshee.log
          log_type: juggler-banshee-log
        - file: juggler/leader/leader.log
          log_type: juggler-leader-log
