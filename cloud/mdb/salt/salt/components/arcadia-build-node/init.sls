{% set user = 'robot-pgaas-ci' %}
{% set group = salt['user.primary_group'](user) %}

/home/{{ user }}/.ssh:
    file.directory:
        - user: {{ user }}
        - makedirs: True
        - mode: 700

/home/{{ user }}/.ssh/id_ed25519:
    file.managed:
        - contents_pillar: data:arcadia-build-node:{{ user }}.key
        - user: {{ user }}
        - mode: 600
        - require:
            - file: /home/{{ user }}/.ssh

checkout-ya:
    cmd.run:
        - name: svn cat svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/ya | python - clone
        - runas: {{ user }}
        - group: {{ group }}
        - creates:
            - /home/{{ user }}/arcadia/ya
        - require:
            - file: /home/{{ user }}/.ssh/id_ed25519
            - file: /home/{{ user }}/.ya/ya.conf

/home/{{ user }}/.ya_token:
    file.managed:
        - contents_pillar: data:arcadia-build-node:{{ user }}.ya_token
        - user: {{ user }}
        - mode: 600
        - require:
            - file: /home/{{ user }}/.ssh

/home/{{ user }}/.ya/ya.conf:
    file.managed:
        - contents: |
            oauth_token_path = "/home/{{ user }}/.ya_token"
            tools_cache_size = "5GiB"
            symlinks_ttl = "4h"
            cache_size = "25GiB"
        - user: {{ user }}
        - mode: 600
        - makedirs: True
        - require:
            - file: /home/{{ user }}/.ya_token

checkout-mdb:
    cmd.run:
        - name: /home/{{ user }}/arcadia/ya make -j0 --checkout cloud/mdb
        - runas: {{ user }}
        - group: {{ group }}
        - cwd: /home/{{ user }}/arcadia
        - creates:
            - /home/{{ user }}/arcadia/cloud/mdb
        - require:
            - cmd: checkout-ya

/etc/cron.d/periodic-arcadia-checkout:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/periodic-arcadia-checkout
        - require:
            - cmd: checkout-mdb
