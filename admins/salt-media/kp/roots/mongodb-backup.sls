include:
  - templates.mongodb-backup

/root/.s3cfg:
  file.managed:
    - source: salt://common/files/s3cfg
    - template: jinja
    - context:
      s3_access_key: {{ salt['pillar.get']('s3cmd:s3_access_key') }}
      s3_secret_key: {{ salt['pillar.get']('s3cmd:s3_secret_key') }}
    - user: root
    - group: root
    - mode: 0400
    - makedirs: True

install_mongo_backup_pkgs:
  pkg.installed:
    - pkgs:
      - corba-mongo-backup
      - yandex-media-s3cmd

