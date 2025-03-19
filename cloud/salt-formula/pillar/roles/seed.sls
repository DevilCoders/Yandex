push_client:
  instances:
    billing:
      enabled: True
      files:
        - name: /var/lib/yc/compute/accounting/image.log
          log_type: billing-compute-image
        - name: /var/lib/yc/compute/accounting/snapshot.log
          log_type: billing-compute-snapshot
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
    resource-manager:
      enabled: True
      proto: rt
      files:
        - name: /var/log/yc/resource-manager/access.log
          ident: yc_api
          log_type: yc-api-request-log
