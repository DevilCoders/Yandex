nginx-ping-to-backend:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 20
    - command: /usr/bin/curl -ks -q https://localhost/ping/ | grep -q pong && echo "0;OK($(date +%c))" || echo "2;FAILED($(date +%c))"

{% set sub_domain={
    "testing": "tst.",
    "stress": "load.",
  }.get("yandex-environment", "") %}
{% set limit = 3 if grains['yandex-environment'] in ['production', 'prestable'] else 0 %}

www-touch-api-5xx:
  monrun.present:
  - command: "/usr/sbin/tskv /var/log/nginx/access.log -h touch-api.{{ sub_domain }}kinopoisk.ru -s |/usr/bin/awk '/total_rps/ {total=$2};/5xx/ {err=$2} END {if(total!=0){prc=err/total*100; if(prc>=5){print \"2; CRIT: \"prc\"% 5xx errors (\"err\" from \"total\" total)\"} else if({{limit}}<prc && prc<5){print \"1; WARNING: \"prc\"% 5xx errors (\"err\" from \"total\" total)\"} else print \"0; OK\"} else print \"0; OK. zero total\"}'"
  - execution_interval: 60
  - execution_timeout: 30
  - type: kinopoisk

www-touch-api-ping:
  monrun.present:
  - command: /usr/bin/jhttp.sh -t 10 --scheme https -p 443 -o '-k --user-agent Yandex' -n touch-api.kinopoisk.ru -u '/ping'
  - execution_interval: 60
  - execution_timeout: 30
  - type: kinopoisk

www-touch-api-actor:
  monrun.present:
  - command: /usr/bin/jhttp.sh -t 10 --scheme https -p 443 -o '-k --user-agent Yandex' -n touch-api.kinopoisk.ru -u '/web/1.0.0/getKPPeopleDetailView?cityID=1&peopleID=558099'
  - execution_interval: 60
  - execution_timeout: 30
  - type: kinopoisk

monrun_php-fpm:
  monrun.present:
    - name: php-fpm
    - type: kinopoisk
    - command : /usr/bin/daemon_check.sh 'php-fpm.*master.process'

kp-auth:
  monrun.present:
    - execution_interval: 60
    - type: kinopoisk
    - command: /usr/local/bin/kp-auth-check.sh
