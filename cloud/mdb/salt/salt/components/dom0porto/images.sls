/data/images/trusty.tar.bz2:
    file.managed:
        - makedirs: True
        - source: http://dist.yandex.ru/images/ubuntu/ubuntu-14.04.tar.bz2
        - source_hash: http://dist.yandex.ru/images/ubuntu/ubuntu-14.04.tar.bz2.md5
        - require_in:
            - test: images-ready

/data/images/bionic.tar.bz2:
    file.managed:
        - makedirs: True
        - source: http://dist.yandex.ru/images/ubuntu/ubuntu-18.04.tar.bz2
        - source_hash: http://dist.yandex.ru/images/ubuntu/ubuntu-18.04.tar.bz2.md5
        - require_in:
            - test: images-ready

{% if salt['pillar.get']('data:dom0porto:use_dbaas_dom0_images', False) %}
s3cmd:
    pkg.installed

/root/.s3cfg:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/s3cfg' }}
        - mode: '0600'
        - require:
            - pkg: s3cmd

/etc/cron.d/dom0-images-sync:
    file.managed:
        - source: salt://{{ slspath + '/conf/dom0_images_sync.cron' }}
        - require:
            - file: /root/.s3cfg

/etc/logrotate.d/dom0-images-sync:
    file.managed:
        - source: salt://{{ slspath + '/conf/dom0_images_sync.logrotate' }}
        - require:
            - file: /root/.s3cfg

/etc/cron.yandex/dom0-images-sync.sh:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/dom0_images_sync.sh' }}
        - mode: '0755'
        - require:
            - file: /root/.s3cfg

initial-images-sync:
    cmd.wait:
        - name: flock -o /tmp/dom0-images-sync.lock timeout 3600 /etc/cron.yandex/dom0-images-sync.sh
        - watch:
            - file: /etc/cron.yandex/dom0-images-sync.sh
        - require_in:
            - file: /etc/cron.yandex/heartbeat.py
            - test: images-ready
{% endif %}
