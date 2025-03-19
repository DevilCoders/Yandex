{%- set default_folder = 'sites-available' %}
{%- set default_folder_link = 'sites-enabled' %}
{% if nginx_folders is defined %}
  {%- set default_folder = nginx_folders.folder %}
  {%- set default_folder_link = nginx_folders.link %}
{% endif %}

{%- for config_name in nginx_configs %}
/etc/nginx/{{ default_folder }}/{{ config_name }}:
  file.managed:
    - makedirs: True
    - source: salt://{{ slspath }}/nginx-sites/{{ config_name }}
    - template: jinja
    - require:
      - file: /etc/nginx/conf.d/logformat.conf

{%- if default_folder_link != None %}
/etc/nginx/{{ default_folder_link }}/{{ config_name }}:
  file:
    - symlink
    - makedirs: True
    - target: /etc/nginx/{{ default_folder }}/{{ config_name }}
    - force: True
    - require:
      - file: /etc/nginx/{{ default_folder }}/{{ config_name }}
{%- endif %}
{%- endfor %}

nginx_reload-{{ sls }}:
  service.running:
    - name: nginx
    - reload: True
{%- for config_name in nginx_configs %}
    - watch:
      - file: /etc/nginx/{{ default_folder }}/{{ config_name }}
      {%- if default_folder_link != None %}
      - file: /etc/nginx/{{ default_folder_link }}/{{ config_name }}
      {%- endif %}
{%- endfor %}
