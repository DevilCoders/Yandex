{% set hostname = grains['nodename'] %}
{% set upstream = salt['grains.get']('cluster_map:hosts:%s:slb_adapter:upstream' % hostname)  %}

include:
  - .interface
  - nginx

/etc/nginx/stream-sites-enabled/slb-adapter-http-https:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.stream.slb-adapter-http-https
    - template: jinja
    - defaults:
        upstream: {{ upstream }}
    - require:
      - file: /etc/nginx/stream-sites-enabled

/etc/nginx/sites-enabled/ping-81.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.ping-81.conf

/etc/resolv.conf:
  file.symlink:
    - target: /run/resolvconf/resolv.conf
    - force: True

/usr/local/bin/dns-check.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/dns-check.sh
    - mode: 755

/etc/cron.d/dns-check:
  file.managed:
    - source: salt://{{ slspath }}/files/dns-check.cron

/etc/systemd/system/nginx.service:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.service
    - require:
      - file: /usr/local/bin/dns-check.sh
      - file: /etc/cron.d/dns-check

/etc/cron.d/wd-nginx:
  file.managed:
    - source: salt://{{ slspath }}/files/wd-nginx.cron

service.systemctl_reload:
  module.run:
    - onchanges:
      - file: /etc/systemd/system/nginx.service

# remove obsolete monrun check
/home/monitor/agents/modules/nginx-alive.sh:
  file.absent

{% from slspath+"/monitoring.yaml" import monitoring %}
{% include "common/deploy_mon_scripts.sls" %}

extend:
  nginx:
    service:
      - watch:
        - file: /etc/nginx/stream-sites-enabled/slb-adapter-http-https
        - file: /etc/nginx/sites-enabled/ping-81.conf
        - module: service.systemctl_reload
