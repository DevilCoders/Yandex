{% set yaenv = grains['yandex-environment'] %}
{% if yaenv in ['development'] %}
{% set host_fqdn = grains['fqdn'] %}
{% set dev_user = salt['pillar.get']('dev_users_to_host:'+host_fqdn) %}
{% set servername = dev_user+'.'+pillar.domains.kp_web %}
{% else %}
{% set servername = pillar.domains.kp_web %}
{% endif %}


include:
  - templates.nginx

cond.d/realip.conf:
  yafile.managed:
    - name: /etc/nginx/conf.d/realip.conf
    - source: salt://{{ slspath  }}/files/etc/nginx/conf.d/realip.conf
    - watch_in:
      - service: nginx

sites-enabled/php-fpm.conf:
  yafile.managed:
    - name: /etc/nginx/sites-enabled/10-php-fpm.conf
    - source: salt://{{ slspath  }}/files/etc/nginx/sites-enabled/10-php-fpm.conf
    - template: jinja
    - context:
      servername: {{ servername  }}
      rootdir: /home/www/kinopoisk.ru
    - watch_in:
      - service: nginx

{% for conf in [
    '25-internal-api.conf',
    '25-comment-api.conf',
] %}
sites-enabled/{{ conf  }}:
  yafile.managed:
    - name: /etc/nginx/sites-enabled/{{ conf  }}
    - source: salt://{{ slspath  }}/files/etc/nginx/sites-enabled/{{ conf  }}
    - template: jinja
    - context:
      rootdir: /home/www/kinopoisk.ru
    - watch_in:
      - service: nginx
{% endfor %}
