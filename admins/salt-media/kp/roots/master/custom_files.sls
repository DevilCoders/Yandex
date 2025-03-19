/var/log/kinopoisk:
  file.directory:
    - user: www-data
    - group: www-data

/var/log/cron:
  file.directory:
    - user: www-data
    - group: www-data

kp_cron_logrotate:
  file.managed:
    - name: /etc/logrotate.d/kinopoisk_cron
    - source: salt://{{slspath}}/files/etc/logrotate.d/kinopoisk_cron
    - mode: 644

# dirs
{% for dir in ['im','images','trailers'] %}
/home/www/static.kinopoisk.ru/{{ dir }}:
  file.directory:
    - user: www-data
    - group: www-data
    - makedirs: true
# symlinks
/home/www/kinopoisk.ru/{{ dir }}:
  file.symlink:
    - target: ../static.kinopoisk.ru/{{ dir }}
{% endfor %}

/home/www/static.kinopoisk.ru/js:
  file.symlink:
    - target: ../kinopoisk.ru/js

/home/www/static.kinopoisk.ru/shab:
  file.symlink:
    - target: ../kinopoisk.ru/shab


{% if grains['yandex-environment'] == 'production' %}
# ssh key for www-data: need to ssh from master to static and conv
/var/www/.ssh/id_rsa:
  file.managed:
    - contents: {{salt.pillar.get("www_data:id_rsa")|json}}
    - makedirs: true
    - user: www-data
    - group: www-data
    - mode: 600
{% endif %}

/etc/console-distributed-flock.json:
  yafile.managed:
    - source: salt://{{ slspath }}/files/bconsole/console-distributed-flock.json
    - user: root
    - group: root
    - mode: 644

/etc/yandex/bconsole/kinsole.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/bconsole/kinsole.yaml
    - user: root
    - group: root
    - mode: 644
    - makedirs: true

/etc/sudoers.d/bconsole:
  file.managed:
    - source: salt://{{ slspath }}/files/bconsole/bconsole_sudoers
    - user: root
    - group: root
    - mode: 440

{% if grains['yandex-environment'] == 'production' %}
/etc/yandex/check-kp-test-prod-migrations.json:
  file.managed:
    - contents: {{ salt['pillar.get']('kp-migrations:check_conf') | json }}
    - makedirs: true
    - user: root
    - group: root
    - mode: 600

/usr/local/bin/s3-config-graphdata.yaml:
  file.managed:
    - contents: {{ pillar['s3config-graphdata_secret']|json }}
    - user: www-data
    - group: www-data
    - mode: 0440
    - makedirs: True
{% endif %}

/usr/local/bin/s3-config-static.yaml:
  file.managed:
    - contents: {{ pillar['s3config-static_secret']|json }}
    - user: www-data
    - group: www-data
    - mode: 0440
    - makedirs: True

python-pkg:
  pkg.installed:
    - pkgs:
      - python-pip
      - python3-pip
      - python3-boto3
      - python3-pymysql
      - python3-termcolor
      - python3-yaml

api_scripts_pkgs:
  pkg.installed:
    - names:
      - python3-yaml
      - yandex-kinoposk-api-scripts
