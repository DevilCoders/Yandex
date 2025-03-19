
/etc/karapace/karapace-config.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/karapace-config.json
        - mode: 644
        - watch_in:
            - service: karapace-service

karapace-service:
    service.running:
        - name: karapace
        - enable: True
