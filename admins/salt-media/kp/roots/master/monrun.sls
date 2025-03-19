db_unavail:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - command: timetail -t common -n60 /var/log/php/php.log | perl -nE'BEGIN{$r=$o=0}if(/SQL Error from conn/){$o++}}{if($o>5){$r=1}if($o>15){$r=2}say"$r;$o fails per 60 sec"'
    - type: kinopoisk

# services
monrun_nginx:
  monrun.present:
    - name: nginx
    - type: kinopoisk
    - command : /usr/bin/daemon_check.sh nginx

monrun_php-fpm:
  monrun.present:
    - name: php-fpm
    - type: kinopoisk
    - command : /usr/bin/daemon_check.sh 'php-fpm.*master.process'

# www
www.kinopoisk.ru_main_redirect:
  monrun.present:
    - execution_interval: 120
    - type: kinopoisk
    - command: "/usr/bin/jhttp.sh -o '--user-agent jhttp' -n www.kinopoisk.ru -p 8080 -s http -e 'code: 301'"

nginx-www-5xx:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - type: kinopoisk
    - command: /usr/bin/codechecker.pl --format tskv --log /var/log/nginx/access.log --host '{{ pillar.domains.kp_web }}' --code '5..'

nginx-bo-5xx:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - type: kinopoisk
    - command: /usr/bin/codechecker.pl --format tskv --log /var/log/nginx/access.log --host '{{ pillar.domains.kp_bo }}' --code '5..'

kp-migrations:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 10
    - type: kinopoisk
    - command: /usr/local/sbin/kp-migrations check

# CADMIN-6911
lsyncd-daemon:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 10
    - type: kinopoisk
    - command: /usr/local/sbin/lsyncd-daemon-check.py

# CADMIN-6977
cron-errors:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 20
    - type: kinopoisk
    - command: /usr/local/sbin/kp-check-cron.pl

# CADMIN-7851
upload-s3:
  monrun.present:
    - execution_interval: 60
    - type: kinopoisk
    - command: timetail -t java -n 3600 /var/log/s3/upload_s3*.log /var/log/s3/upload_s3*.log.1 | perl -ne 'if (/Failed to upload/) { $count++; if ($count >= 2) { print "2;Too many errors while uploading to S3. See /var/log/s3/upload_s3.log\n"; exit; } } } { print "0;OK\n";'

# CADMIN-7324
{% if grains['yandex-environment'] == 'production' %}
kp-migrations-versions:
  monrun.present:
    - execution_interval: 600
    - execution_timeout: 10
    - type: kinopoisk
    - command: /usr/bin/pkg-versions-env-check.pl /etc/yandex/check-kp-test-prod-migrations.json
{% endif %}

# CADMIN-9632
/etc/config-monrun-cert-check/cert_expires_config.sh:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - mode: 0644
    - source: salt://{{ slspath }}/files/cert_expires_config.sh
