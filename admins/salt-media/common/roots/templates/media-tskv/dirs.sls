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

/etc/nginx/ssl:
  file.directory:
    - dir_mode: 755
    - user: root
