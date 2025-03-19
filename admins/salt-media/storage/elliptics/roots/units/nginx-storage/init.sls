{% from 'units/nginx-storage/ns_redirect.sls' import ns_video_list %}
{% from 'units/nginx-storage/ns_redirect.sls' import ns_list_8181 %}
{% from 'units/nginx-storage/ns_redirect.sls' import ns_list_80443 %}

include:
  - templates.unistat-lua

{% for file in pillar.get('nginx-storage-config-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
    - watch_in:
      - service: nginx
{% endfor %}

{% for file in pillar.get('nginx-storage-dirs-root', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('nginx-storage-dirs-www-data', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: www-data
    - group: root
    - makedirs: True
{% endfor %}

{% for ns in ns_video_list %}
/etc/nginx/include/rvideo/{{ ns.name }}.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/redirect_video_template.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - makedirs: True
    - defaults:
        ns_name: {{ ns.name }}
        token: {{ ns.token }}
    - watch_in:
      - service: nginx
{% endfor %}

{% for ns in ns_list_8181 %}
/etc/nginx/include/rns_8181/{{ ns.name }}.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/redirect_template.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - makedirs: True
    - defaults:
        ns_name: {{ ns.name }}
        token: {{ ns.token }}
        cors: {{ ns.get('cors') }}
    - watch_in:
      - service: nginx
{% endfor %}

{% for ns in ns_list_80443 %}
/etc/nginx/include/rns_80443/{{ ns.name }}.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/nginx/include/redirect_template.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - makedirs: True
    - defaults:
        ns_name: {{ ns.name }}
        token: {{ ns.token }}
        cors: {{ ns.get('cors') }}
    - watch_in:
      - service: nginx
{% endfor %}

/etc/nginx/include/rns_80443/tfb_testing.conf:
  file.absent:
    - name: /etc/nginx/include/rns_80443/tfb_testing.conf

nginx:
  service:
    - running
    - reload: True
    - require:
      - pkg: nginx
  user:
    - name: www-data
    - present
    - system: True
    - groups:
      - www-data
    - require:
      - group: www-data
  group:
    - present
    - name: www-data
    - system: True
  pkg:
    - installed
    - pkgs:
      - nginx
