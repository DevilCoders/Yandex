rsyncd:
  lookup:
    mopts:
      type: kinopoisk
    shares:
      'backup':
      - path: /opt/backup
      - read only: false
      - timeout: 300
