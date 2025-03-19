{% from slspath + "/map.jinja" import nginx with context %}

{% for dir in '/var/spool/nginx','/var/cache/nginx' %}
{{ dir }}:
  file.directory:
    - dir_mode: 755
    - user: www-data
    - group: www-data
{% endfor %}

/var/cache/nginx/fastcgi-cache:
  file.directory:
    - dir_mode: 755
    - user: root

/var/cache/nginx/proxy-cache:
  file.directory:
    - dir_mode: 755
    - user: root

/var/cache/nginx/spool:
  file.directory:
    - dir_mode: 755
    - user: www-data


client-temp-create-dir:
  file.directory:
    {%- if nginx.client_body_temp_path is defined %}
    - name: {{ nginx.client_body_temp_path.split(' ')[0] }}
    {%- else %}
    - name: /var/tmp/nginx/client-temp
    {%- endif %}
    - makedirs: True
    - dir_mode: 755
    - user: www-data
