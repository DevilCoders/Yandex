include:
    - components.mdb-metrics
    - .mdb-metrics

redis-mdb-metrics-cleanup:
    file.absent:
        - names:
            - /etc/mdb-metrics/conf.d/available/instance_userfault_broken.conf
            - /etc/mdb-metrics/conf.d/enabled/instance_userfault_broken.conf
        - watch_in:
            - service: mdb-metrics-service
