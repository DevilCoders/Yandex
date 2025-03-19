{% for file in pillar.get('logdaemon-conf-files', []) %}
{{file}}:
  file.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
    - watch_in:
      - service: logdaemon
{% endfor %}

{% for file in pillar.get('logdaemon-exec-files', []) %}
{{file}}:
  file.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
    - template: jinja
    - watch_in:
      - service: logdaemon
{% endfor %}

{% for file in pillar.get('logdaemon-exec-root-files', []) %}
{{file}}:
  file.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
    - watch_in:
      - service: logdaemon
{% endfor %}

{% for file in pillar.get('logdaemon-dirs', []) %}
{{file}}:
  file.directory:
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
{% endfor %}

logdaemon:
  service:
    - running
    - reload: False
