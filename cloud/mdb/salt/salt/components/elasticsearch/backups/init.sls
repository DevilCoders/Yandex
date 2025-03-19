{% set config = {
    'enabled': salt.mdb_elasticsearch.auto_backups_enabled(),
    'cluster_id': salt.pillar.get('data:dbaas:cluster_id'),
    'index_details': salt.mdb_elasticsearch.version_ge('7.13'),
    's3': {
        'access_key_id': salt.pillar.get('data:s3:access_key_id'),
        'access_secret_key': salt.pillar.get('data:s3:access_secret_key'),
        'endpoint': salt.pillar.get('data:s3:endpoint'),
        'bucket': salt.mdb_elasticsearch.s3bucket()
    },
    'elastic': {
        'user': salt.pillar.get('data:elasticsearch:users:mdb_admin:name'),
        'password': salt.pillar.get('data:elasticsearch:users:mdb_admin:password')
    },
    'repository': {
        'name': 'yc-automatic-backups',
        'settings': {
            'client': 'yc-automatic-backups',
            'base_path': 'backups',
            'bucket': salt.mdb_elasticsearch.s3bucket()
        }
    },
    'policy': {
        'name': 'yc-automatic-backups',
        'settings': {
            'name': 'snapshot',
            'schedule': '0 14 * * * ?',
            'repository': 'yc-automatic-backups',
            'config': {
                'indices': [ '*' ]
            },
            'retention': {
                'expire_after': '14d',
                'max_count': 24*14,
                'min_count': 72
            }
        }
    },
    'extensions': {
        'active': salt.mdb_elasticsearch.active_extension_ids(),
        'all': salt.mdb_elasticsearch.all_extension_ids(),
    }
} %}


backup-packages:
    pkg.installed:
        - pkgs:
            - python3-boto3

backups-user:
    user.present:
        - name: elasticsearch-backups
        - groups:
            - es-certs
            - elasticsearch
        - system: True

/usr/local/yandex/es_backups.py:
    file.managed:
        - source: salt://{{ slspath }}/backups.py
        - mode: 755
        - makedirs: True

/etc/yandex/mdb-elasticsearch/backups.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/backups.conf
        - user: elasticsearch-backups
        - group: monitor
        - mode: 440
        - makedirs: True
        - defaults:
            data: {{ config | yaml }}

/etc/cron.d/es-backups:
{% if salt.mdb_elasticsearch.auto_backups_enabled() %}
    file.managed:
        - source: salt://{{ slspath }}/backups.cron
        - template: jinja
        - mode: 644
{% else %}
    file.absent
{% endif %}

/etc/logrotate.d/es-backups:
    file.managed:
        - source: salt://{{ slspath }}/backups.logrotate
        - mode: 644
        - makedirs: True


elasticsearch-backups-req:
    test.nop

backups-repository:
    mdb_elasticsearch.ensure_repository:
        - reponame: {{ config.repository.name }}
        - settings: {{ config.repository.settings | yaml }}
        - require:
            - test: elasticsearch-backups-req
 
backups-policy:
    mdb_elasticsearch.ensure_snapshot_policy:
        - policy_name: {{ config.policy.name }}
        - settings: {{ config.policy.settings | yaml }}
        - require:
            - test: elasticsearch-backups-req
