include:
  - db.common.mysql-monitoring-4

{%- if grains['yandex-environment'] in ["production"] %}
mysql_configurator_4_configs:
  file.managed:
    - makedirs: True
    - names:
      - /etc/mysql-configurator/backup.yaml:
        - source: salt://{{ slspath }}/files/mysql-configurator-4/backup.yaml
      - /etc/cron.d/mysql-backup:
        - source: salt://{{ slspath }}/files/mysql-configurator-4/mysql-backup
      - /root/.s3cfg:
        - source: salt://db/common/files/s3cfg
        - template: jinja
        - context:
          s3_access_key: {{ salt['pillar.get']('s3cmd:s3_access_key') }}
          s3_secret_key: {{ salt['pillar.get']('s3cmd:s3_secret_key') }}
        - user: root
        - group: root
        - mode: 0400
        - makedirs: True
{% endif %}

yandex-media-s3cmd:
  pkg.installed

libzstd:
  pkg.installed

zstd:
  pkg.installed
