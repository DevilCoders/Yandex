rsyncd:
  lookup:
    mopts:
      type: kinopoisk
    shares:
      static:
      - 'path': '/home/www/static.kinopoisk.ru'
      - 'read only': 'no'
      - 'auth users': 'www-data'
      - 'secrets file': '/etc/rsyncd.secrets'
