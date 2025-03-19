{% set is_prod = grains['yandex-environment'] in ['production', 'prestable'] %}
{% set rotate = salt['pillar.get']('php:logrotate:rotate', 48) %}

# if fpm configured
{% if salt.pillar.get("php:fpm") %}
{{ salt.pillar.get("php:fpm_conf") }}:
  yafile.managed:
    - source: salt://common/files/php/php-fpm.conf.tpl
    - template: jinja
    - context:
      config: {{ pillar.php.fpm }}
    - watch_in:
      - cmd: php-fpm-configtest
{% endif %}

# if php-cli configured
{% if salt.pillar.get("php:ini") %}
{{ salt.pillar.get("php:ini_conf") }}:
  yafile.managed:
    - source: salt://common/files/php/php.ini.tpl
    - template: jinja
    - context:
      config: {{ pillar.php.ini }}
    - watch_in:
      - cmd: php-fpm-configtest
{% if salt.pillar.get("php:version") == "5.6" %}
{% set php_cli = pillar.php.ini %}
{% do php_cli.update(pillar.php.cli) %}
/etc/php/5.6/cli/php.ini:
  yafile.managed:
    - source: salt://common/files/php/php.ini.tpl
    - follow_symlinks: false
    - template: jinja
    - context:
      config: {{ php_cli }}
/etc/php/7.3/cli/php.ini:
  yafile.managed:
    - source: salt://common/files/php/php.ini.tpl
    - follow_symlinks: false
    - template: jinja
    - context:
      config: {{ php_cli }}
{% endif %}
{% endif %}

/etc/logrotate.d/php:
  file.managed:
    - source: salt://common/files/logrotate.d/php
    - template: jinja
    - context:
      rotate: {{ rotate }}

/var/log/php:
  file.directory:
    - user: www-data
    - group: www-data
    - mode: 755
    - makedirs: true

/var/log/php73:
  file.directory:
    - user: www-data
    - group: www-data
    - mode: 755
    - makedirs: true

/var/www/html/index.html:
  file.managed:
    - makedirs: true
    - contents: |
        <meta http-equiv="refresh" content="0; url=http://www.kinopoisk.ru" />

{% if not is_prod %}  # opcache www only on nonproduction
opcache_www:
{% if "opcache.enable" in salt.pillar.get("php:ini:opcache") %}
  file.recurse:
    - name: /var/www/html/opcache
    - source: salt://common/files/php/www/opcache
{% else %}
  file.absent:
    - name: /var/www/html/opcache
{% endif %}
{% endif %}

{% if "opcache.blacklist_filename" in salt.pillar.get("php:ini:opcache") %}
{{ salt.pillar.get("php:ini:opcache:opcache.blacklist_filename") }}:
  file.managed:
    - source: salt://common/files/php/opcache-blacklist.txt
    - template: jinja
    # - context: {{ pillar.php.ini}}
    - watch_in:
      - cmd: php-fpm-configtest
{% endif %}

# Check for configured ubic
{% if salt.pillar.get("php:fpm_ubic_name") == "" %}
service_php_fpm:
  service.running:
    - name: {{ pillar.php.fpm_service_name }}
    - enable: true
    - reload: true
    - sig: 'php-fpm: master process'
    - watch:
      - cmd: php-fpm-configtest
{% else %}
/etc/init.d/php:
  file.managed:
    - source: salt://common/files/ubic/php.init
    - mode: 0755

/etc/ubic/service/php:
  file.managed:
    - source: salt://common/files/ubic/php.tpl
    - template: jinja
    - context:
      config: {{ pillar.php }}
    - makedirs: True

disable_stock_php_fpm:
  service.dead:
    - name: {{ pillar.php.fpm_service_name }}
    - enable: False

ubic_php_fpm:
  service.running:
    - name: {{ pillar.php.fpm_ubic_name }}
    - enable: True
    - sig: 'php-fpm: master process'
{% endif %}

php-fpm-configtest:
  cmd.wait:
    - name: {{ pillar.php.fpm_bin }} -t

cp1251-locale:
  file.managed:
    - name: /var/lib/locales/supported.d/rucp
    - contents: |
        ru_RU.CP1251 CP1251
  cmd.run:
    - name: locale-gen
    - onchanges:
      - file: /var/lib/locales/supported.d/rucp

include:
  - .xdebug
  - templates.sudoers

# need for run php with non-root user (cannot load php.ini config)
change_etcphp56_cfgdir_perm:
  file.directory:
    - name: /etc/php/5.6
    - mode: 0755
change_etcphp_cfgdir_perm:
  file.directory:
    - name: /etc/php
    - mode: 0755
