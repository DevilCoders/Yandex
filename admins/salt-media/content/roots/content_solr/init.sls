{% set yaenv = grains['yandex-environment'] %}

/etc/cron.d/solr-restarting:
  file.managed:
    - source: salt://content_solr/files/solr-restarting
    - user: root
    - group: root
    - mode: 644
    - replace: True

/root/.s3cfg:
  file.managed:
    - contents: {{ pillar['s3cmd']|json }}
    - user: root
    - group: root
    - mode: 0400

yandex-media-s3cmd:
  pkg.installed

yandex-media-solr-backup-restore:
  pkg.installed

/etc/cron.d/solr-backup:
  yafile.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://content_solr/files/solr-backup-cron
    - require:
      - pkg: yandex-media-solr-backup-restore

{% if yaenv in ['testing'] %}
/etc/config-monrun-cpu-check/config.yml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        warning: 85
        critical: 90
{% endif %}
