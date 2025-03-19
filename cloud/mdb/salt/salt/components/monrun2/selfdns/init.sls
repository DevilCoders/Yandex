monrun-selfdns-scripts:
    file.absent:
        - name: /usr/local/yandex/monitoring/selfdns_logs.py
        - watch_in:
            - cmd: monrun-jobs-update

monrun-selfdns-confs:
    file.absent:
        - name: /etc/monrun/conf.d/selfdns_logs.conf
        - watch_in:
            - cmd: monrun-jobs-update
