{% if salt.pillar.get('data:dbaas:vtype') == 'compute' and salt.pillar.get('data:service_account_id', None) != None %}
/etc/cron.d/clickhouse-s3-credentials:
    file.managed:
        - source: salt://components/clickhouse/conf/clickhouse-s3-credentials.cron
        - template: jinja
        - mode: 644

get-new-token-required:
  test.configurable_test_state:
    - changes: {{ salt.mdb_clickhouse.s3_credentials_config_recreate_required() }}
    - result: True
    - comment: S3 credentials going to be recreated

get-iam-token:
    cmd.run:
        - name: /usr/bin/ch-s3-credentials update --endpoint '{{ salt.pillar.get('data:object_storage:endpoint', None) }}'
        - onchanges:
            - file: /etc/cron.d/clickhouse-s3-credentials
            - test: get-new-token-required

{% else %}
/etc/cron.d/clickhouse-s3-credentials:
    file.absent
/etc/clickhouse-server/config.d/s3_credentials.xml:
    file.absent
{% endif %}
