push_client:
  instances:
    access-service:
      enabled: True
      proto: rt
      files:
        - name: /var/log/yc/access-service/access.log
          ident: yc_api
          log_type: yc-api-request-log
        - name: /var/log/yc/access-service/server.log
          ident: yc_api
          log_type: yc-server-log
          pipe: /etc/yandex/statbox-push-client/conf.d/serverlog.py
