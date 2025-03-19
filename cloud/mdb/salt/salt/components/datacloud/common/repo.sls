/etc/apt/apt.conf.d/s3:
    file.managed:
        - source: salt://{{ slspath }}/conf/apt_s3.conf
        - template: jinja
        - makedirs: True
        - mode: 644
        - require_in:
            - cmd: repositories-ready
