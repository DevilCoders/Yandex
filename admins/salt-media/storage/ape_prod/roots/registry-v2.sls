/usr/share/perl5/Ubic/Service/SimpleDaemonTorrents.pm:
  file.managed:
    - source: salt://registry-v2/usr/share/perl5/Ubic/Service/SimpleDaemonTorrents.pm

/root/.docker/config.json:
  file.managed:
    - source: salt://docker/config.json
    - makedirs: True

docker:
  pkg.installed:
    - pkgs:
      - docker-engine=1.11.2-0~trusty
    - skip_suggestions: True
    - install_recommends: False
/usr/local/bin/mds_ns_check.py:
  file.managed:
    - source: salt://common-files/usr/local/bin/mds_ns_check.py
    - mode: 755
/usr/lib/yandex-graphite-checks/enabled/registry_ns_check.sh:
  file.managed:
    - source: salt://registry-v2/usr/lib/yandex-graphite-checks/enabled/registry_ns_check.sh
    - mode: 755
/etc/apt/sources.list.d/docker-engine.list:
  file.managed:
    - source: salt://common-files/etc/apt/sources.list.d/docker-engine.list
/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://registry-v2/etc/monrun/conf.d/
/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://registry-v2/etc/nginx/sites-enabled/
/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://logrotate.d/nginx
/etc/docker-robot-keyfile:
  file.managed:
    - source: salt://registry-v2/etc/docker-robot-keyfile
    - user: root
    - mode: 600
/etc/nginx/ssl:
  file.recurse:
    - source: salt://certs/registry-v2
    - user: root
    - file_mode: 600
/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://registry-v2/etc/nginx/nginx.conf
/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: "salt://registry-v2/etc/yandex/loggiver/loggiver.pattern"
/etc/nginx/conf.d/01-accesslog.conf:
  file.managed:
    - source: salt://registry-v2/etc/nginx/conf.d/01-accesslog.conf
/etc/sysctl.d:
  file.recurse:
    - source: salt://registry-v2/etc/sysctl.d/
/etc/docker-registry-torrents/config.yaml:
  file.managed:
    - source: salt://registry-v2/etc/docker-registry-torrents/config.yaml
/etc/docker-distribution.yml:
  file.managed:
    - source: salt://registry-v2/etc/docker-distribution.yml
/etc/docker-idm.json:
  file.managed:
    - source: salt://registry-v2/etc/docker-idm.json
/etc/apt/sources.list.d:
  file.recurse:
    - source: salt://registry-v2/etc/apt/sources.list.d/
/etc/ubic/service:
  file.recurse:
    - source: salt://registry-v2/etc/ubic/service/
/etc/logrotate.d/ubic:
  file.managed:
    - source: salt://registry-v2/etc/logrotate.d/ubic
