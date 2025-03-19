{%- from slspath + "/vars.sls" import config with context -%}

{% set files = [
    '/etc/logrotate.d/s3-goose-maintain',
    '/etc/s3-goose/main-maintain.json',
    '/etc/s3-goose/maintain.json',
    '/etc/ubic/service/s3-goose-maintain.json',
    '/usr/share/perl5/Ubic/Service/S3GooseMaintain.pm'
  ]
%}

{% set exefiles = [
    '/usr/local/bin/s3-maintain-report-alert.sh'
  ]
%}

{% set dirs = [
    '/var/log/s3/goose-maintain'
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

{% set components = [
    "goose-maintain"
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

/usr/local/yasmagent/CONF/agent.s3goosemaintenance.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/yasmagent/CONF/agent.s3goosemaintenance.conf
    - user: root
    - group: root
    - mode: 644

restart-yasmagent-yasm-s3goosemaintenance:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /usr/local/yasmagent/CONF/agent.s3goosemaintenance.conf

s3-goose-maintain:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker maintenance maintenance"
    - execution_interval: 300
    - execution_timeout: 30
    - type: s3

s3-maintain-report-alert:
  monrun.present:
    - command: "/usr/local/bin/s3-maintain-report-alert.sh"
    - execution_interval: 300
    - execution_timeout:  60
    - type: s3
