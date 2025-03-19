{% for file in pillar.get('logdaemon-conf-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in pillar.get('logdaemon-exec-files', []) %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: monitor
    - group: monitor
    - makedirs: True
    - template: jinja
    - watch_in:
      - service: logdaemon
{% endfor %}

{% for file in pillar.get('logdaemon-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - require:
        - pkg: nginx
{% endfor %}

{% for file in pillar.get('logdaemon-initd-links', []) %}
symlink-{{file}}:
  file.symlink:
    - name: {{file}}
    - target: /etc/init.d/logdaemon
{% endfor %}

logdaemon:
  service:
    - running
    - reload: False

python3-yaml:
  pkg:
    - installed

python3-setproctitle:
  pkg:
    - installed
