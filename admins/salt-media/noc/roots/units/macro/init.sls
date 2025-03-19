/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://{{ slspath }}/etc/nginx/sites-enabled/
    - template: jinja

/etc/macros/macros.yaml:
  file.managed:
    - source: salt://{{ slspath }}/etc/macros/macros.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
