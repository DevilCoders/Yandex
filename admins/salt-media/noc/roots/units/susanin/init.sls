{% set unit = "susanin" %}

include:
  - units.nginx_conf
  - templates.certificates

/etc/nginx/sites-enabled/susanin.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/sites-enabled/susanin.conf
    - template: jinja
    - makedirs: True
    
/etc/susanin/:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/susanin/
    - template: jinja
    - file_mode: 644
    - makedirs: True

/var/spool/susanin/tvm-cache/:
  file.directory:
    - mode: 644
    - makedirs: True

