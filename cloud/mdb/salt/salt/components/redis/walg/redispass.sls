{% from slspath ~ "/map.jinja" import walg with context %}

mdb-user-redispass:
    file.managed:
        - name: {{ salt.mdb_redis.get_redispass_file(walg.user) }}
        - source: salt://{{ slspath }}/conf/redispass
        - template: jinja
        - mode: 600
        - user: {{walg.user}}
        - group: s3users
