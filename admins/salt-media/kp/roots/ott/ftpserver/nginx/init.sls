include:
  - templates.nginx

{% for config in ['s3-mds-metadata-server', 'ottftp'] %}
/etc/nginx/sites-available/{{ config }}.conf:
  file.managed:
    - source: salt://{{ slspath }}/{{ config }}.conf
    - makedirs: True
    - mode: 0644

/etc/nginx/sites-enabled/{{ config }}.conf:
  file.symlink:
    - target: /etc/nginx/sites-available/{{ config }}.conf
    - makedirs: True
    - force: True
{% endfor %}