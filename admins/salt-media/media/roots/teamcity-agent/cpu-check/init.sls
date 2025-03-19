/etc/config-monrun-cpu-check/config.yml:
    file.managed:
        - makedirs: true
        - source: salt://{{ slspath }}/config.yml
