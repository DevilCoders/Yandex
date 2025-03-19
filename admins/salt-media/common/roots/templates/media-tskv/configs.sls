/etc/nginx/conf.d/01-accesslog-tskv.conf:
  file.managed:
    - makedirs: True
    - source: salt://templates/media-tskv/files/01-accesslog-tskv.conf
    - template: jinja
