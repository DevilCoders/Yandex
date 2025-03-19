/etc/monrun/conf.d/monrun_root_password.conf:
    file.managed:
        - makedirs: true
        - source: salt://{{ slspath }}/config
