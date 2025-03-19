{%- from slspath + "/vars.sls" import config with context -%}

{% set files = [
  '/etc/logrotate.d/s3-goose-cleanup',
  '/etc/s3-goose/cleanup.json',
  '/etc/s3-goose/main-cleanup.json',
  '/etc/s3-goose/yandexCAs.pem',
  '/etc/ubic/service/s3-cleanup.json',
  '/usr/share/perl5/Ubic/Service/S3Cleanup.pm'
  ]
%}

{% set exefiles = [
  '/usr/local/bin/monrun-s3-cleanup-check.py'
  ]
%}

{% set dirs = [
  '/var/log/s3/goose-cleanup'
  ]
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

s3-cleanup:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker cleanup cleanup"
    - execution_interval: 300
    - execution_timeout: 30
    - type: s3

{% set components = [
    "goose-cleanup"
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
