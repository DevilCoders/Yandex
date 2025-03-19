push_client:
  enabled: True
  # basic configs parameters

  instances:
    # override basic configs parameters for concrete instance of push-client
    billing:
      enabled: True
      files:
        - name: /var/log/s3/print-log-broker-byte-sec-log/log-broker.log
          log_type: billing-object-storage
        - name: /var/log/nginx/tskv.log
          pipe: /usr/local/bin/tskv_to_json_billing.py
          log_type: billing-object-requests
    s3proxy:
      enabled: True
      files:
        - name: /var/log/nginx/tskv.log
          ident: cloud_prod_storage
          log_type: s3proxy-nginx-access-log

nginx-s3-certs:
  - /etc/nginx/ssl/s3-yc-test.yandex.net
  - /etc/nginx/ssl/s3-private

nginx-s3-config-files:
  - /etc/nginx/s3-arc/map_allowed_origins.conf
  - /etc/nginx/s3-arc/method_options.conf
  - /etc/nginx/s3-arc/error_page_503.conf
  - /etc/nginx/s3-arc/proxy_options.conf
  - /etc/nginx/s3-arc/blacklist_referer.conf
  - /etc/nginx/s3-lua/s3-solomon-metrics.lua
  - /etc/nginx/s3-lua/s3-idm-solomon-metrics.lua
  - /etc/nginx/s3-lua/worker.lua
  - /etc/nginx/conf.d/accesslog-tskv.conf
  - /etc/nginx/sites-enabled/s3-mds-yc-idm-arc.conf
  - /etc/nginx/sites-enabled/s3-solomon.conf
  - /etc/nginx/sites-enabled/s3-website.conf
  - /etc/nginx/sites-enabled/s3-mds-proxy-arc.conf
  - /etc/logrotate.d/s3-proxy

s3-services-config:
  s3-proxy-arc:
    yc_pkg: s3-mds-proxy-arc
    files:
      - /etc/s3-mds-proxy-arc/s3-proxy-arc.conf
      - /etc/s3-mds-proxy-arc/s3-yc-idm-arc.conf
      - /etc/s3/s3-update-counters.conf
      - /etc/s3/s3-update-cloud-counters.conf
      - /etc/s3/clean-storage-delete-queue.conf
      - /etc/s3/s3-kikimr-clean-in-flight.conf
      - /etc/s3/print-log-broker-byte-sec-log.conf
      - /etc/s3/s3-lifecycle-scheduler.conf
      - /etc/s3/s3-lifecycle-worker.conf

s3-service-log-dirs:
  - /var/log/s3/print-log-broker-byte-sec-log/
  - /var/log/s3-kikimr-update-counters
  - /var/log/s3-kikimr-update-cloud-counters
  - /var/log/s3/clean-storage-delete-queue
  - /var/log/s3/s3-kikimr-clean-in-flight
  - /var/log/s3/lf/worker
  - /var/log/s3/lf/scheduler
  - /var/log/s3-mds-yc-idm-arc
  - /var/log/s3-mds-proxy-arc

s3-service-scripts:
  - /usr/local/bin/tskv_to_json_billing.py

default_buckets_cloud_quota: 25
default_cloud_total_alive_size_quota: 5497558138880


logdaemon-exec-files:
  - /usr/local/bin/logdaemon.py

logdaemon-exec-root-files:
  - /etc/systemd/system/logdaemon.service
  - /usr/bin/daemon_check.sh
  - /usr/local/sbin/autodetect_environment

logdaemon-conf-files:
  - /etc/logdaemon/config.yaml
  - /etc/logrotate.d/logdaemon
  - /etc/monrun/conf.d/daemon_check_logdaemon.conf

logdaemon-dirs:
  - /var/log/logdaemon
  - /var/run/logdaemon
