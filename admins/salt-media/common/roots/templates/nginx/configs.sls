{% from slspath + "/map.jinja" import nginx with context %}
include:
  - .services

/etc/nginx/nginx.conf:
  file.managed:
    - source: {{ nginx.nginx_config }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - dir_mode: 755
    - watch_in:
      - service: {{ nginx.service }}

/etc/nginx/conf.d/main.conf:
  file.managed:
    - source: {{ nginx.confd_config }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - dir_mode: 755
    - template: jinja
    - context:
      params: {{ nginx.confd_params }}
    - watch_in:
      - service: {{ nginx.service }}

/etc/nginx/events/main.conf:
  file.managed:
    - source: {{ nginx.events_config }}
    - user: root
    - group: root
    - mode: 644
    - dir_mode: 755
    - makedirs: True
    - template: jinja
    - context:
      params: {{ nginx.events_params }}
    - watch_in:
      - service: {{ nginx.service }}

/etc/nginx/main/main.conf:
  file.managed:
    - source: {{ nginx.main_config }}
    - user: root
    - group: root
    - mode: 644
    - dir_mode: 755
    - makedirs: True
    - template: jinja
    - context:
      params: {{ nginx.main_params }}
    - watch_in:
      - service: {{ nginx.service }}

{% if salt["pillar.get"]("nginx:limit_req_zones", False) %}
/etc/nginx/conf.d/limit_req_zones.conf:
  file.managed:
    - name: /etc/nginx/conf.d/limit_req_zones.conf
    - template: jinja
    - source: {{ nginx.limit_req_zones_config }}
{% endif %}

{% if nginx.accesslog_config %}
/etc/nginx/conf.d/accesslog.conf:
  file.managed:
    - source: {{ nginx.accesslog_config }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - template: jinja
    - context:
      log_params: {{ nginx.log_params|default(False) }}
    - watch_in:
      - service: {{ nginx.service }}

{# migration from media-tskv template #}
/etc/nginx/conf.d/01-accesslog-tskv.conf:
  file.absent
{% endif %}

{% if nginx.get("confd_addons", False) %}
{% for name, text in nginx.confd_addons.items() %}
/etc/nginx/conf.d/{{name}}.conf:
  file.managed:
    - user: root
    - group: root
    - makedirs: True
    - watch_in:
      - service: {{ nginx.service }}
    - contents: |
        {{text|indent(8)}}
{% endfor %}
{% endif %}

/etc/nginx/sites-available/00-default.conf:
  file.managed:
    - source: {{ nginx.vhost_default_config }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - dir_mode: 755
    - template: jinja
    - context:
      listens: {{ nginx.listens|json }}
      def_cert: {{ nginx.vhost_default_cert }}
    - watch_in:
      - service: {{ nginx.service }}

{% if nginx.vhost_default %}
/etc/nginx/sites-enabled/00-default.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/00-default.conf
{% endif %}

{% for name, params in nginx.listens.items() %}
{% if params.get("enabled", True) %}
/etc/nginx/{{ name }}:
  file.managed:
    - source: salt://templates/nginx/files/etc/nginx/listen.tmpl
    - template: jinja
    - makedirs: True
    - dir_mode: 755
    - context:
        params: {{ params|json }}
    - watch_in:
      - service: {{ nginx.service }}
{% endif %}
{% endfor %}

/etc/logrotate.d/nginx:
  file.absent

/etc/logrotate.d/logrotate-nginx:
  file.managed:
    - source: {{ nginx.logrotate_config }}
    - template: jinja
    - context:
      logrotate: {{ nginx.logrotate }}
    - user: root
    - group: root
    - mode: 644
    - dir_mode: 755
    - makedirs: True

generate-dhparam:
  cmd.run:
    - name: openssl dhparam -out /etc/nginx/ssl/dhparam.pem 2048
    - require:
      - file: /etc/nginx/ssl/yandexca.pem
    - require_in:
      - service: {{ nginx.service }}
    - unless: test -s /etc/nginx/ssl/dhparam.pem

/etc/nginx:
  file.directory:
    - dir_mode: 0755
    - makedirs: True
/etc/nginx/ssl:
  file.directory:
    - dir_mode: 0700
    - require:
      - file: /etc/nginx
/etc/nginx/ssl/yandexca.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/ssl/yandexca.pem
    - mode: 0400
    - require:
      - file: /etc/nginx/ssl
