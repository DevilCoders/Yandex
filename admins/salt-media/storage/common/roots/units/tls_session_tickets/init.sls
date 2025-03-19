{% set unit = 'tls_session_tickets' %}
{% set config = pillar.get(unit) %}

include:
  - templates.certificates

{%for dir, files in config.items()%}
{{dir}}:
  file.directory:
    - user: root
    - makedirs: True

{% for file, contents in files.items() %}
{{dir}}/{{file}}:
  file.decode:
    - encoding_type: base64
    - encoded_data: {{contents | json}}
    - watch_in:
      - service: certificate_packages
    - require:
      - file: {{dir}}
{% endfor %}
{% endfor %}

tls_tickets_rotate_cron:
  cron.present:
    - name: sleep $((RANDOM\%900)) && salt-call state.apply units.{{unit}} -l quiet queue=True
    - minute: 34
    - identifier: tls_session_tickets_update

PATH:
  cron.env_present:
    - value: /usr/sbin:/usr/bin:/sbin:/bin

MAILTO:
  cron.env_present:
    - value: '""'
