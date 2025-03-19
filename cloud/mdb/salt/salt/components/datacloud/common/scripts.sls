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

/usr/local/yandex/mount_data_directory.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/mount_data_directory.sh
        - makedirs: True
        - mode: 700
