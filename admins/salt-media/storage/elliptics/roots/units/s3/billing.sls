{%- from slspath + "/vars.sls" import config with context -%}

{% set files = [
  '/etc/logrotate.d/s3-billing',
  '/etc/s3-goose/billing.json',
  '/etc/s3-goose/main-billing.json',
  '/etc/ubic/service/s3-billing.json',
  '/usr/share/perl5/Ubic/Service/S3Billing.pm',
  ]
%}

{% set exefiles = [
  '/etc/init.d/s3-billing',
  '/usr/local/bin/s3-billing-bad-value.sh'
  ]
%}

{% set dirs = [
  '/var/log/s3/billing/'
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

s3-billing:
  monrun.present:
    - command: "/usr/local/bin/s3-goose-checker billing billing"
    - execution_interval: 300
    - execution_timeout: 45
    - type: s3

s3-billing-bad-value:
  monrun.present:
    - command: "/usr/local/bin/s3-billing-bad-value.sh"
    - execution_interval: 300
    - execution_timeout:  60
    - type: s3

s3-billing-bad-value-size:
  monrun.present:
    - command: "/usr/local/bin/s3-billing-bad-value.sh bad"
    - execution_interval: 300
    - execution_timeout:  60
    - type: s3

{% set components = [
    "billing"
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

