/etc/apt/trusted.gpg.d/ondrej-php-gpg.key:
  file.managed:
    - source: salt://{{ slspath }}/files/gpg.key
  cmd.run:
    - name: apt-key add /etc/apt/trusted.gpg.d/ondrej-php-gpg.key
    - onchanges:
      - file: /etc/apt/trusted.gpg.d/ondrej-php-gpg.key

ondrej-php-ppa:
  pkgrepo.managed:
    - name: deb http://mirror.yandex.ru/mirrors/launchpad/ondrej/php bionic main
    - dist: bionic
    - file: /etc/apt/sources.list.d/ondrej-ubuntu-php-bionic.list
    - refresh_db: true

php7.4:
  pkg.installed:
    - require:
      - pkgrepo: ondrej-php-ppa

/etc/yandex/dc:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/dc
    - template: jinja

/etc/invapi.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/invapi.yml
    - template: jinja

/etc/rtapi2rr.yml:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/rtapi2rr.yml
    - template: jinja
{% if grains['yandex-environment'] == 'production' %}
/etc/rtapi2rr.local.yml:
  file.absent
/etc/invapi.local.yml:
  file.absent
{% endif %}


/etc/default/config-caching-dns:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/default/config-caching-dns

{% if not pillar["rt_with_mysql_on_host"]|default(false) %}
/root/.my.cnf:
  file.managed:
    - source: salt://{{ slspath }}/files/my.cnf
    - template: jinja
{% endif %}
/etc/tvmtool/tvmtool.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/tvmtool.conf
    - template: jinja
    - makedirs: True

/usr/share/rt-yandex/plugins/scripts/switchvlans/cisco.secrets.php:
  file.managed:
    - source: salt://{{ slspath }}/files/cisco.secrets.php
    - template: jinja
    - makedirs: True

/etc/nginx/:
  file.recurse:
    - source: salt://{{ slspath }}/files/nginx/
    - template: jinja
    - makedirs: True

/var/cache/nginx/export-map-nat64:
  file.directory:
    - makedirs: True
    - user: nobody

/etc/logrotate.d/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/

/etc/php/7.4/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/php/7.4/
    - template: jinja
    - makedirs: True

yandex-passport-tvmtool:
  service.running:
    - enable: True
  pkg.installed:
    - pkgs:
      - yandex-passport-tvmtool
