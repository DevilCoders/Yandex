{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% else %}
{% endif %}

{%- from slspath + "/vars.sls" import config with context -%}

ug_s3proxy:
  group:
    - name: s3proxy
    - present
    - system: True
  user:
    - name: s3proxy
    - present
    - system: True
    - home: /var/run/s3proxy
    - shell: /bin/false
    - groups:
      - s3proxy
    - require:
      - group: s3proxy

/etc/default/s3-goose-checker.json:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/default/s3-goose-checker.json
    - mode: 644
    - user: s3proxy
    - group: s3proxy
    - makedirs: True

/etc/default/s3-goose-shell.json:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/default/s3-goose-shell.json
    - mode: 644
    - user: s3proxy
    - group: s3proxy
    - makedirs: True

/etc/default/s3-restart-options:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/default/s3-restart-options
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja

/etc/s3-goose:
  file.directory:
    - user: s3proxy
    - group: s3proxy
    - dir_mode: 755
    - require:
      - user: s3proxy
      - group: s3proxy

/etc/s3-goose/iam_private.pem:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/s3-goose/iam_private.pem
    - user: s3proxy
    - group: s3proxy
    - mode: 600
    - makedirs: True
    - template: jinja

/etc/s3-goose/allCAs.pem:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/yandex-certs/allCAs.pem
    - mode: 444
    - user: s3proxy
    - group: s3proxy
    - makedirs: True

{% for file in [
    '/usr/local/bin/s3-restart-status.sh',
    '/usr/local/bin/s3-restart-lock-age.sh',
] %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

s3-goose-restart:
  monrun.present:
    - command: /usr/local/bin/s3-restart-status.sh
    - execution_interval: 60
    - execution_timeout: 20
    - type: s3

s3-goose-restart-lock:
  monrun.present:
    - command: /usr/local/bin/s3-restart-lock-age.sh
    - execution_interval: 60
    - execution_timeout: 20
    - type: s3

{% for file in [
    '/etc/s3-goose/common.json',
    '/etc/s3-goose/postgres.json'
] %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}
