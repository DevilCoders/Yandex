{% set vtype = salt.pillar.get('data:dbaas:vtype') %}
{% set default_pool = salt.pillar.get('data:image-releaser:compute:destination:pooling:size', 3) %}
{% set default_folder = salt.pillar.get('data:image-releaser:compute:destination:folder', 'NO_DEFAULT') %}
{% set default_keep_images = salt.pillar.get('data:image-releaser:compute:destination:keep_images', 2) %}

include:
    - components.monrun2.image-releaser

mdb-image-releaser-pkgs:
    pkg.installed:
       - pkgs:
            - mdb-image-releaser: '1.9268650'
            - yazk-flock

/opt/yandex/mdb-image-releaser:
    file.directory:
        - user: mdb-image-releaser
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-image-releaser-user

/opt/yandex/mdb-image-releaser/mdb-image-releaser.yaml:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath + '/conf/mdb-image-releaser.yaml' }}
        - mode: 755
        - user: mdb-image-releaser
        - makedirs: True
        - require:
            - pkg: mdb-image-releaser-pkgs
            - file: /opt/yandex/mdb-image-releaser

mdb-image-releaser-user:
    user.present:
        - fullname: MDB Image-releaser user
        - name: mdb-image-releaser
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True

/etc/logrotate.d/mdb-image-releaser:
    file.managed:
        - source: salt://{{ slspath }}/conf/logrotate.conf
        - mode: 644
        - makedirs: True

/var/log/mdb-image-releaser:
    file.directory:
        - user: mdb-image-releaser
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-image-releaser-user

/var/run/mdb-image-releaser:
    file.directory:
        - user: mdb-image-releaser
        - makedirs: True
        - mode: 755
        - require:
            - user: mdb-image-releaser-user

/etc/cron.d/mdb-image-releaser:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-image-releaser.cron
        - mode: 644
        - template: jinja

{% for db in salt['pillar.get']('data:images') %}

{% set alias = db.get('alias', db.name) %}

/opt/yandex/mdb-image-releaser/zk-flock-{{ alias }}.json:
    file.managed:
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - template: jinja
        - user: mdb-image-releaser
        - mode: 640
        - require:
            - file: /opt/yandex/mdb-image-releaser
        - context:
            name: {{ alias }}

/etc/cron.yandex/mdb-image-releaser-{{ alias }}.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/mdb-image-releaser.sh
        - mode: 755
        - template: jinja
        - require:
            - user: mdb-image-releaser-user
            - pkg: mdb-image-releaser-pkgs
        - require_in:
            - file: /etc/cron.d/mdb-image-releaser
        - context:
            name: {{ db.name }}
            mode: {{ vtype }}
            folder: {{ db.get('folder_id', default_folder) }}
            os: {{ db.get('os', 'linux') }}
            product_ids: {{ db.get('product_ids', []) }}
            pool_size: {{ db.get('pool_size', default_pool) }}
            keep_images: {{ db.get('keep_images', default_keep_images) }}
            no_check: {{ db.get('no_check', False) }}
            check_host: {{ db.get('e2e_check_host', False) }}
            check_service: {{ db.get('e2e_check_service', False) }}
{% endfor %}
