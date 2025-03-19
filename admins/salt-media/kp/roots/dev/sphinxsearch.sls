sphinxsearch_pkg:
  pkg.installed:
    - name: sphinxsearch

sphinxsearch_service:
  service.running:
    - name: sphinxsearch
    - enable: true

/home/sphinx:
  file.directory:
    - user: sphinx

remove_config:
  file.absent:
    - name: /etc/sphinxsearch/sphinx.conf

/etc/sphinxsearch/sphinx.conf:
  file.symlink:
    - target: /home/www/kinopoisk.ru/config/sphinx/sphinx_conf.php

/usr/bin/indexer --rotate --quiet --all:
  cron.present:
    - user: root
    - special: '@daily'