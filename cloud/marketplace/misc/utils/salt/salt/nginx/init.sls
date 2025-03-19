nginx:
  pkg.installed:
    - name: nginx
    - version: 1.14.2-1.yandex.22

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://nginx/files/nginx.nginx


/etc/nginx/support_ssl.conf:
  file.managed:
    - source: salt://nginx/files/ssl.conf
    - template: jinja
    - defaults:
      hostname: {{ pillar['support-api']['hostname'] }}

/var/cache/nginx/:
  file.directory:
    - user: root
    - group: root
    - mode: '0777'

/etc/nginx/ssl/:
  file.directory:
    - user: root
    - group: root
    - mode: '0700'

{%- for ext in ['crt', 'pem'] %}
/etc/nginx/ssl/{{pillar['common']['hostname']}}.{{ ext }}:
  file.managed:
    - user: root
    - group: root
    - mode: '0600'
    - contents: |
        {{ pillar['certs'][ext]|indent(8) }}

/etc/nginx/ssl/{{pillar['support-api']['hostname']}}.{{ ext }}:
  file.managed:
    - user: root
    - group: root
    - mode: '0600'
    - contents: |
        {{ pillar['certs-support'][ext]|indent(8) }}
{%- endfor %}


{%- for conf in ['10-default', '11-healthcheck', '99-support-console'] %}
/etc/nginx/sites-enabled/{{ conf }}.conf:
  file.managed:
    - source: salt://nginx/files/{{ conf }}.nginx
    - template: jinja
{%- endfor %}

