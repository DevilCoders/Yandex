{% set yaenv = grains['yandex-environment'] %}

/etc/nginx/sites-available:
  file.recurse:
    - source: salt://{{slspath}}/files/nginx/sites-available
    - template: jinja
    - context:
        yaenv: {{yaenv}}

/etc/nginx/sites-enabled/kino-ui-tv-config-nginx:
  file.symlink:
    - target: /etc/nginx/sites-available/desktop-env-{{yaenv}}
    - require:
      - file: /etc/nginx/sites-available

/etc/nginx/sites-enabled/kino-ui-tv-touch-phone-config-nginx:
  file.symlink:
    - target: /etc/nginx/sites-available/touch-env-{{yaenv}}
    - require: 
      - file: /etc/nginx/sites-available 

/etc/nginx/sites-enabled/kino-ui-tv-pda:
  file.symlink:
    - target: /etc/nginx/sites-available/pda
    - require:
      - file: /etc/nginx/sites-available

/etc/nginx/sites-enabled/01-ukraine-proxy-map.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/01-ukraine-proxy-map.conf
    - require:
      - file: /etc/nginx/sites-available

