php73_pkgs:
  pkg.installed:
    - pkgs:
      - php-common
      - php7.3-memcached
      - php7.3-imagick
      - php7.3-memcache
      - php7.3-bcmath
      - php7.3-bz2
      - php7.3-cli
      - php7.3-common
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
      - php7.3-redis
      - php7.3-soap
      - php7.3-sqlite3
      - php7.3-xml
      - php7.3-xmlrpc
      - php7.3-xsl
      - php7.3-zip

php7.3-fpm_stop:
  service.dead:
    - name: php7.3-fpm

php7.3-fpm_disable:
  service.disabled:
    - name: php7.3-fpm

/usr/local/bin/additional_php_modules.sh:
  file.managed:
    - makedirs: True
    - mode: 755
    - contents: |
        extension_dir=$(php7.3 -i | grep '^extension_dir' | awk '{ print $5 }')

        update-alternatives --set php /usr/bin/php7.3
        pecl install translit-0.7.1
        pecl install timezonedb

        for dir in cli fpm; do
            mkdir -p /etc/php/7.3/${dir}/conf.d
            for extension in libpuzzle translit timezonedb; do
                echo "extension=${extension}.so" > /etc/php/7.3/${dir}/conf.d/25-${extension}.ini
            done
        done

        touch /var/tmp/additional_php_modules.done

install_modules:
  cmd.run:
    - name: bash /usr/local/bin/additional_php_modules.sh
    - required:
      - file: /usr/local/bin/additional_php_modules.sh
    - unless: "[[ -f /var/tmp/additional_php_modules.done ]]"
    - require:
      - pkg: php73_pkgs

