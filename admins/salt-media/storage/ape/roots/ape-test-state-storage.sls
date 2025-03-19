/etc/cron.d/:
  file.recurse:
    - source: salt://ape-test-state/etc/cron.d/

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://ape-test-state/etc/nginx/nginx.conf

/var/cache/nginx/:
  file.directory:
    - dir_mode: 755
    - user: www-data
    - group: www-data

/etc/nginx/sites-enabled/:
  file.recurse:
    - source: salt://ape-test-state/etc/nginx/sites-enabled/

/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://ape-test-state/etc/monrun/conf.d/

/usr/local/bin/:
  file.recurse:
    - source: salt://ape-test-state/usr/local/bin/
    - file_mode: '0755'

/etc/apt/sources.list.d/dist.list:
  file.managed:
    - source: salt://ape-test-state/etc/apt/sources.list.d/dist.list

/etc/darkvoice/darkvoice.yaml:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-test-state-storage.darkvoice.yaml

/etc/default:
  file.recurse:
    - source: salt://ape-test-state/etc/default

/etc/ubic/service:
  file.recurse:
    - source: salt://ape-test-state/etc/ubic/service/

/etc/logrotate.d/darkvoice:
  file.managed:
    - source: salt://ape-test-state/etc/logrotate.d/darkvoice

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://logrotate.d/nginx
