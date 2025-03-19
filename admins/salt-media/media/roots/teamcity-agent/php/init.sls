/etc/php5/php.ini:
  file.managed:
    - source: salt://{{ slspath }}/php.ini
    - user: root
    - group: root
    - mode: 644

/etc/php5/conf.d/xdebug.ini:
  file.managed:
    - source: salt://{{ slspath }}/xdebug.ini
    - user: root
    - group: root
    - mode: 644

/etc/php/5.6/fpm/conf.d/20-xdebug.ini:
  file:
    - absent
/etc/php/5.6/mods-available/xdebug.ini:
  file:
    - absent
/etc/php/5.6/cli/conf.d/20-xdebug.ini:
  file:
    - absent

/etc/php5/conf.d:
  file.directory:
    - makedirs: True
    - require_in:
      - file: /etc/php5/php.ini
      - file: /etc/php5/conf.d/xdebug.ini

php_packages:
  pkg.installed:
    - pkgs:
      - php7.3
      - php7.3-cli
      - php7.3-common
      - php7.3-bcmath
      - php7.3-bz2
      - php7.3-curl
      - php7.3-dba
      - php7.3-dev
      - php7.3-fpm
      - php7.3-gd
      - php7.3-imap
      - php7.3-intl
      - php7.3-json
      - php7.3-mbstring
      - php7.3-mysql
      - php7.3-opcache
      - php7.3-readline
      - php7.3-soap
      - php7.3-sqlite3
      - php7.3-xml
      - php7.3-xmlrpc
      - php7.3-zip

update_alternative_set_php_5_6:
  cmd.run:
    - name: update-alternatives --set php /usr/bin/php5.6
    - unless: php -v | grep 5.6

