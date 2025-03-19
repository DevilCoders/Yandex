{% set yaenv = grains['yandex-environment'] %}
{% set sites = ['kino-vod-bo', 'yandex-kinoplatform-www-env-'+yaenv ] %}

nginx_sites_available:
  file.recurse:
    - name: /etc/nginx/sites-available/
    - source: salt://{{slspath}}/files/nginx/
    - template: jinja
    - file_mode: 644

{% for site in sites %}
/etc/nginx/sites-enabled/{{site}}:
  file.symlink:
    - source: /etc/nginx/sites-available/{{ site }}
    - target: /etc/nginx/sites-enabled/{{ site }}
    - watch_in:
      - module: test-nginx-config
{% endfor %}

# test nginx config
test-nginx-config:
  module.wait:
    - name: nginx.configtest
