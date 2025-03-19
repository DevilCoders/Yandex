{% for file in pillar.get('nginx-proxy-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

{% for file in pillar.get('nginx-proxy-monrun-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - watch_in: monrun-regenerate
{% endfor %}

{% for file in pillar.get('nginx-proxy-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - require:
        - pkg: nginx
{% endfor %}

{% for file in pillar.get('nginx-proxy-helper-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
{% endfor %}

{% for file in pillar.get('nginx-proxy-user', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: www-data
    - group: www-data
    - makedirs: True
    - require:
        - pkg: nginx
{% endfor %}

{% for file in pillar.get('nginx-proxy-exec-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
{% endfor %}

{% for file in pillar.get('nginx-proxy-config-files', []) %}
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

{% for file in pillar.get('nginx-proxy-available-files', []) %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - user: root
    - group: root
    - mode: 644
    - template: jinja

{{ file | replace("sites-available", "sites-enabled")}}:
  file.symlink:
    - target: {{ file }}
{% endfor %}

{% for file in pillar.get('nginx-proxy-template-files', []) %}
{{ file }}:
  file.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - user: root
    - group: root
    - mode: 644

/usr/bin/nginx_proxy_cache_template.pl:
  cmd.wait:
    - watch:
      - file: {{ file }}
    - require_any:
      - pkg: config-nginx-cache-template
      - file: /usr/bin/nginx_proxy_cache_template.pl
  file.exists:
    - unless:
      - pkg: config-nginx-cache-template
{% endfor %}

/usr/share/lua/5.1/:
  file.recurse:
    - source: salt://{{ slspath }}/files/usr/share/lua/5.1/
    - user: root
    - group: root
    - dir_mode: 755
    - file_mode: 644

config-nginx-cache-template:
  pkg:
    - installed
    - unless:
      - file: /usr/bin/nginx_proxy_cache_template.pl


{% for file in pillar.get('nginx-proxy-ubic-services', []) %}
{{ file }}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - user: s3proxy
    - group: s3proxy
    - mode: 644
{% endfor %}

/var/tmp/nginx:
  file.directory:
    - user: www-data
    - group: www-data
    - dir_mode: 775
    - makedirs: True

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
