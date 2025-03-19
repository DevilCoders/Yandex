push_client:
  instances:
    resource-manager:
      enabled: True
      proto: rt
      files:
        - name: /var/log/yc/resource-manager/access.log
          ident: yc_api
          log_type: yc-api-request-log
