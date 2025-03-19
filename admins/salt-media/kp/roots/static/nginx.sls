{% set fqdn = grains['fqdn'] %}
{% set rdc = grains['conductor']['root_datacenter'] %}

# server related configs
/etc/nginx/sites-enabled/10-st.kp.yandex.net.conf:
  yafile.managed:
    - source: salt://static/files/etc/nginx/sites-enabled/10-st.kp.yandex.net.conf
    - template: jinja
    - watch_in:
      - service: nginx

/etc/nginx/include/global-maps.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/global-maps.conf
    - makedirs: True

/etc/nginx/include/upstreams.conf:
  file.managed:
    - source: salt://static/files/etc/nginx/include/upstreams.conf
    - template: jinja
    - makedirs: True
/etc/nginx/include/keepalive.conf:
  file.managed:
    - source: salt://static/files/etc/nginx/include/keepalive.conf
    - makedirs: True

/home/www/static.kinopoisk.ru/.tmp:
  file.directory:
    - user: www-data
    - group: www-data
    - mode: 755
    - makedirs: True

static.kinopoisk.ru_dir:
  file.directory:
    - name: /home/www/static.kinopoisk.ru
    - user: www-data
    - group: www-data

symlink-js:
  file.symlink:
    - name: /home/www/static.kinopoisk.ru/js
    - target: ../kinopoisk.ru/js
symlink-shab:
  file.symlink:
    - name: /home/www/static.kinopoisk.ru/shab
    - target: ../kinopoisk.ru/shab
symlink-fonts:
  file.symlink:
    - name: /home/www/static.kinopoisk.ru/fonts
    - target: ../kinopoisk.ru/fonts
symlink-public:
  file.symlink:
    - name: /home/www/static.kinopoisk.ru/public
    - target: ../kinopoisk.ru/public

htpasswd:
  file.managed:
    - name: /etc/nginx/.htpasswd
    - contents: {{salt.pillar.get('basic_auth:google')|json}}
