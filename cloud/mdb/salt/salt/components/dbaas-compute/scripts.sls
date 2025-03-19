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

/usr/local/yandex/data_move.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/data_move.sh
        - template: jinja
        - makedirs: True
        - mode: 700

/usr/local/yandex/md_data_move.sh:
    file.absent

/etc/cron.d/mdbuild:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdbuild.cron
        - makedirs: True

/usr/local/sbin/mdbuild.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/mdbuild.sh
        - makedirs: True
        - mode: 700

/usr/local/sbin/mount_data_directory.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/mount_data_directory.sh
        - makedirs: True
        - mode: 700
