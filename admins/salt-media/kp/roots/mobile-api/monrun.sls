nginx-ping-to-backend:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 20
    - command: /usr/bin/curl -s -q http://localhost/ping/ | grep -q pong && echo "0;OK($(date +%c))" || echo "2;FAILED($(date +%c))"

monrun_nginx:
  monrun.present:
    - name: nginx
    - command: /usr/bin/daemon_check.sh nginx

monrun_php-fpm:
  monrun.present:
    - name: php-fpm
    - type: kinopoisk
    - command : /usr/bin/daemon_check.sh 'php-fpm.*master.process'

nginx-ext-5xx:
  monrun.present:
    - execution_interval: 60
    - execution_timeout: 5
    - type: kinopoisk
    - command: /usr/bin/codechecker.pl --format tskv --log /var/log/nginx/access.log --host '{{ pillar.domains.kp_ext }}' --code '5..'

monrun_ext.kinopoisk.ru_ios:
  monrun.present:
    - name: ext.kinopoisk.ru_ios
    - type: kinopoisk
    - command: /usr/bin/jhttp.sh -o '-H X-TIMESTAMP:1527262785 -H X-SIGNATURE:7193443aaf2e37d0faee750bc1128913' -n ext.kinopoisk.ru -u '/ios/5.0.0/index.php?method=getSpotlightView&uuid=d3de3e0385a9a8f8d350d040afe22b36'

kp-auth:
  monrun.present:
    - execution_interval: 60
    - type: kinopoisk
    - command: /usr/local/bin/kp-auth-check.sh
