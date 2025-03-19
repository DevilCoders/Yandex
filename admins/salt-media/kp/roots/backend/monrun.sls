db_unavail:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - command: timetail -t common -n60 /var/log/php/php.log | perl -nE'BEGIN{$r=$o=0}if(/SQL Error from conn/){$o++}}{if($o>5){$r=1}if($o>15){$r=2}say"$r;$o fails per 60 sec"'
    - type: kinopoisk

# services
backend_monrun_nginx:
  monrun.present:
    - name: nginx
    - command: /usr/bin/daemon_check.sh nginx

monrun_php-fpm:
  monrun.present:
    - name: php-fpm
    - type: kinopoisk
    - command : /usr/bin/daemon_check.sh 'php-fpm.*master.process'

# temporarily dev-hosts not needed monrun checks
{% if salt['grains.get']('yandex-environment') != 'development' %}

# www
nginx-www-5xx:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - type: kinopoisk
    - command: /usr/bin/codechecker.pl --format tskv --log /var/log/nginx/access.log --host '{{ pillar.domains.kp_web }}' --code '5..'
www.kinopoisk.ru_ping:
  monrun.present:
    - execution_interval: 120
    - type: kinopoisk
    - command: /usr/bin/jhttp.sh -t 5 -o '--user-agent jhttp' -u /ping

www.kinopoisk.ru_main_redirect:
  monrun.present:
    - execution_interval: 120
    - type: kinopoisk
    - command: "/usr/bin/jhttp.sh -t 5 -o '--user-agent jhttp' -n {{ pillar.domains.kp_web }} -s http -e 'code: 301'"

{% set to = 15 if grains['yandex-environment'] == 'testing' else 10 %}
{% set slonik = '373314' %}

www.kinopoisk.ru_film:
  monrun.present:
    - execution_interval: 120
    - type: kinopoisk
    - command: /usr/bin/jhttp.sh -t {{to}} -o '--user-agent Yandex' -n {{ pillar.domains.kp_web }} -u '/film/{{slonik}}/'

www.kinopoisk.ru_actor:
  monrun.present:
    - execution_interval: 120
    - type: kinopoisk
    - command: /usr/bin/jhttp.sh -t {{to}} -o '--user-agent Yandex' -n {{ pillar.domains.kp_web }} -u '/name/558099/'

# CADMIN-5082
kp-auth:
  monrun.present:
    - execution_interval: 60
    - type: kinopoisk
    - command: /usr/local/bin/kp-auth-check.sh

# KPDUTY-1703
sql-syntax-error:
  monrun.present:
    - execution_interval: 60
    - type: kinopoisk
    - command: timetail -t java -n 3600 /var/log/php/php-db.log /var/log/php/php-db.log.1 /var/log/php/php-main.log /var/log/php/php-main.log.1 | perl -ne 'if (/"errorNo":1064|1064 You have an error in your SQL syntax/) { print "2;Found an error in SQL syntax. Check /var/log/php/php-db.log or /var/log/php/php-main.log\n"; exit; } } { print "0;OK\n";'

# CADMIN-8986
tinyproxy:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 55
    - type: kinopoisk
    - command: |
        perl -M'LWP::UserAgent' -e '$u = LWP::UserAgent->new; $u->timeout(10); for ($i=1; $i <= 5; $i++) { $r = $u->get("http://localhost:8888/", "Host" => "tinyproxy.stats"); $S{$r->code}++; sleep(1); } printf "%s\n", (exists($S{"200"}) and $S{"200"} <= 3) ? "2;tinyproxy fails" : "0;OK";'

{% endif %}
