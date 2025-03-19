/usr/local/bin/s3_closer:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/s3_closer
        - user: root
        - group: root
        - mode: 754

/usr/local/yandex/pre_restart.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/pre_restart.sh
        - template: jinja
        - makedirs: True
        - mode: 700

/usr/local/yandex/post_restart.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/post_restart.sh
        - template: jinja
        - makedirs: True
        - mode: 700
