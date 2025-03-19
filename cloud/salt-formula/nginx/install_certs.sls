{%- set default_folder = 'ssl' %}
{%- if nginx_certs_folder is defined %}
  {%- set default_folder = nginx_certs_folder %}
{%- endif %}

create_folder-{{ sls }}:
  file.directory:
    - user: root
    - name: /etc/nginx/{{ default_folder }}
    - mode: 755
    - require:
      - yc_pkg: nginx

{%- for cert_name in nginx_certs %}
/etc/nginx/{{ default_folder }}/{{ cert_name }}:
  file.managed:
    - source: salt://{{ slspath }}/nginx-certs/{{ cert_name }}
    - makedirs: True
    - user: root
    - mode: 400
    - replace: False
    - require:
      - yc_pkg: nginx
{% endfor %}

nginx_reload_cers-{{ sls }}:
  service.running:
    - name: nginx
    - enable: True
    - reload: True
{%- for cert_name in nginx_certs %}
    - watch:
      - file: /etc/nginx/{{ default_folder }}/{{ cert_name }}
{%- endfor %}

# NOTE: a way to distribute encrypted cert to a virtual machine
# Better approach is to use decrypted cert which was put into local salt tree
# import_gpg_key:
#   cmd.run:
#     - name: scp -q secdist.yandex.net:/repo/projects/cloud-keys/drzoidberg_priv_to_decrypt.key /dev/stdout  | gpg --import -
#
# gpg --output /etc/nginx/ssl/_.df.cloud.yandex.net.key.pem --decrypt ~/_.df.cloud.yandex.net_key.pem.encrypted:
#   cmd.wait:
#     - watch:
#       - cmd: import_gpg_key
#     - reuqire:
#       - yc_pkg: nginx
