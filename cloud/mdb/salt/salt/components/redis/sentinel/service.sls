redis-sentinel:
    service.running:
        - watch:
            - file: sentinel-config
