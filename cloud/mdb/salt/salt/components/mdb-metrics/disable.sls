mdb-metrics-service-masked:
    service.masked:
        - name: mdb-metrics

mdb-metrics:
    service.dead:
        - require:
           - mdb-metrics-service-masked

/etc/cron.d/wd-mdb-metrics:
    file.absent:
        - require_in:
            - service: mdb-metrics
