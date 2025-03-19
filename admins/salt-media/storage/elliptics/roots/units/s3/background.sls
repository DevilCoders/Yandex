{%- from slspath + "/vars.sls" import config with context -%}

{% set files = [
  '/etc/logrotate.d/s3-goose-background',
  '/etc/s3-goose/background.json',
  '/etc/s3-goose/main-background.json',
  '/etc/ubic/service/s3-goose-background.json',
  '/usr/share/perl5/Ubic/Service/S3Background.pm']
%}

{% set exefiles = [
  '/etc/init.d/s3-goose-background',
  '/usr/local/bin/goose_status_mon.py' ]
%}

{% set dirs = [
  '/var/log/s3/goose-background/',
  '/var/cache/s3-goose-background/' ]
%}

{% for file in files %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in exefiles %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in dirs %}
{{file}}:
  file.directory:
    - mode: 755
    - user: s3proxy
    - group: s3proxy
    - makedirs: True
    - require:
      - user: s3proxy
      - group: s3proxy
{% endfor %}

s3-goose-lc:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker background lc"
    - execution_interval: 120
    - execution_timeout: 30
    - type: s3

s3-goose-sqs:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker background sqs"
    - execution_interval: 120
    - execution_timeout: 30
    - type: s3

# TODO: mb not used
/etc/yandex/s3-secret/v1_cipher_key-base64:
  file.managed:
    - user: s3proxy
    - mode: 600
    - contents: |
        {{ pillar['s3_v1_cipher_key'] }}

/etc/yandex/s3-secret/v1_cipher_key:
  cmd.run:
    - name: 'base64 -d /etc/yandex/s3-secret/v1_cipher_key-base64 > /etc/yandex/s3-secret/v1_cipher_key'
    - onchanges:
      - file: /etc/yandex/s3-secret/v1_cipher_key-base64

{% set components = [
    "goose-background"
] %}

{% for comp in components %}
libmds-{{ comp }}:
  yafile.managed:
    - name: /etc/s3-goose/libmds-{{ comp }}.json
    - source: salt://{{ slspath }}/files/etc/s3-goose/libmds.json
    - template: jinja
    - defaults:
        component_name: "{{ comp }}"
{% endfor %}
