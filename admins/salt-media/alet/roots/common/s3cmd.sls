/root/.s3cfg:
  file.managed:
    - user: root
    - group: root
    - mode: 0400
    - contents: {{ salt['pillar.get']('s3cmd_backup') | json }}
yandex-media-s3cmd:
  pkg.installed
