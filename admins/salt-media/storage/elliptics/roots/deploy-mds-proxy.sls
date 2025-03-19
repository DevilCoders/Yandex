include:
  - units.nginx-proxy
  - templates.mediastorage-mulcagate
  - templates.certificates
  - units.elliptics-cache
  - templates.karl-tls
  - templates/unistat-lua

/var/run/mediastorage:
  file.directory

nginx_proxy_cache_template:
  file.managed:
    - name: /usr/bin/nginx_proxy_cache_template.pl
    - source: salt:///files/deploy-proxies/nginx_proxy_cache_template.pl
    - mode: 755
    - user: root
    - group: root
    - require_in: 
      - file: /usr/bin/nginx_proxy_cache_template.pl

/usr/share/perl5/Ubic/Service/Elliptics/Config.pm:
  file.managed:
    - source: salt:///files/deploy-proxies/Config.pm
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

/usr/share/perl5/Ubic/Service/Elliptics/Simple.pm:
  file.managed:
    - source: salt:///files/deploy-proxies/Simple.pm
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

/etc/ubic/service/elliptics.ini:
  file.managed:
    - source: salt:///files/deploy-proxies/elliptics.ini
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

/etc/logrotate.d/elliptics-storage:
  file.managed:
    - source: salt:///files/deploy-proxies/elliptics-storage.logrotate
