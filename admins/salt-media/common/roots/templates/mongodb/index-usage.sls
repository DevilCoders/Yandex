{% from slspath + "/map.jinja" import mongodb with context %}

mongodb_index_usage_packages:
  pkg.installed:
    - pkgs:
      - heirloom-mailx
      - yandex-media-common-mongodb-index-usage

/etc/yandex/index_usage.yaml:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - dir_mode: 0755
    - source: salt://{{ slspath }}/files/etc/yandex/index_usage.yaml.tmpl
    - template: jinja
    - context:
        config: {{ mongodb.index_usage.config }}

/etc/cron.d/mongodb_index_usage:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/etc/cron.d/mongodb_index_usage.tmpl
    - template: jinja
    - context:
        email: {{ mongodb.index_usage.email }}
        schedule: {{ mongodb.index_usage.schedule }}

/usr/bin/mongodb-index-stat-email.sh:
  file.managed:
    - user: root
    - group: root
    - mode: 0775
    - source: salt://{{ slspath }}/files/usr/bin/mongodb-index-stat-email.sh
