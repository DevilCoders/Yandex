{% from slspath ~ "/map.jinja" import walg, s3 with context %}

/etc/logrotate.d/wal-g:
    file.managed:
        - source: salt://{{ slspath }}/conf/wal-g.logrotate
        - mode: 644
        - template: jinja
        - require:
            - file: /etc/cron.d/wal-g
        - defaults:
            log_dir: {{ walg.logdir}}

walg-logdir:
    file.directory:
        - name: {{walg.logdir}}
        - user: root
        - group: {{walg.group}}
        - mode: 770
        - makedirs: True
        - require:
            - user: mdb-backup-user
        - require_in:
            - file: /etc/cron.d/wal-g

mdb-backup-user:
    user.present:
        - name: {{walg.user}}
        - home: {{walg.homedir}}
        - createhome: True
        - empty_password: False
        - shell: /bin/false
        - system: True
        - groups:
            - s3users
        - require:
            - group: mdb-s3users-group

mdb-s3users-group:
    group.present:
        - name: s3users
        - system: True
        - addusers:
            - monitor
        - require:
            - pkg: yamail-monrun
        - watch_in:
            - service: juggler-client

walg-confdir:
    file.directory:
        - name: {{walg.confdir}}
        - user: root
        - group: s3users
        - mode: 0750
        - makedirs: True
        - require:
            - group: mdb-s3users-group
        - require_in:
            - file: {{walg.confdir}}/wal-g.yaml

{{walg.confdir}}/PGP_KEY:
    file.managed:
        - user: root
        - group: s3users
        - mode: 640
        - contents: |
            {{salt.pillar.get('data:s3:gpg_key') | indent(12)}}
        - require:
            - pkg: walg-packages
            - file: walg-confdir

{{walg.confdir}}/s3cmd.conf:
    file.managed:
        - source: salt://{{ slspath }}/conf/s3cmd.conf
        - template: jinja
        - context:
            s3_endpoint: {{s3.endpoint}}
            s3_access_key_id: {{s3.access_key_id}}
            s3_access_secret_key: {{s3.access_secret_key}}
            s3_use_https: {{s3.get('use_https', True)}}
        - user: root
        - group: s3users
        - mode: 640
        - require:
            - file: walg-confdir


{% if walg.get('zk_hosts') %}
{%  for rname, rpath in walg.zk_root.items() %}
walg-zk-root-{{rname}}:
    zookeeper.present:
        - name: {{ rpath }}
        - value: ''
        - makepath: True
        - hosts: {{ walg.zk_hosts }}
        - require:
            - pkg: walg-packages
        - require_in:
            - file: /usr/local/bin/walg_wrapper.sh

{{walg.confdir}}/zk-flock-{{rname}}.json:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/zk-flock.json
        - defaults:
            zk_hosts: {{ walg.zk_hosts}}
            zk_root: {{ rpath }}
            log_file: {{ walg.logdir}}/{{rname}}.log
        - mode: 644
        - makedirs: True
        - require:
            - pkg: walg-packages
            - file: walg-confdir
        - require_in:
            - file: /usr/local/bin/walg_wrapper.sh
{%  endfor %}
{% endif %}

walg-sudoers:
    file.recurse:
        - name: /etc/sudoers.d
        - file_mode: '0640'
        - template: jinja
        - source: salt://{{ slspath }}/sudoers.d

# walg installed and all configs created
walg-ready:
  test.nop:
    - require:
        - pkg: walg-packages
        - file: walg-logdir
        - file: {{walg.confdir}}/wal-g.yaml
        - file: /usr/local/bin/walg_wrapper.sh
        - user: mdb-backup-user
        - file: {{walg.confdir}}/PGP_KEY
        - file: walg-sudoers

/usr/local/bin/walg_wrapper.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/walg_wrapper.sh
        - user: root
        - group: root
        - mode: 755
        - require:
            - file: {{walg.confdir}}/wal-g.yaml
        - require_in:
            - file: /etc/cron.d/wal-g

/usr/local/bin/redis_cli.sh:
    file.managed:
        - source: salt://{{ slspath }}/conf/redis_cli.sh
        - user: root
        - group: root
        - mode: 755
        - require_in:
            - test: walg-ready

{{walg.confdir}}/backup_src.sh:
    file.absent

extend:
    mdb-user-redispass:
        file.managed:
            - require:
                - user: mdb-backup-user
