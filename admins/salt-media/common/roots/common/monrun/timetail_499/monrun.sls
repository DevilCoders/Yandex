timetail_499:
  monrun.present:
    - command: codechecker.pl -n 300 --code '^499$' -err 0.1  --format tskv --url "^/timetail*" --log /var/log/nginx/access.log
    - execution_interval: 60
    - execution_timeout: 30
    - require:
      - pkg: yandex-media-common-codechecker
