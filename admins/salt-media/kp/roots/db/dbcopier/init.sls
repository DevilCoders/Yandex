mysql-dbcopier:
  pkg.installed

{% set files = {
  'full': '1-5',
  'fast': '0',
  'source': '',
  'destination': '',
} %}

{% for filename in files %}
/etc/mysql-dbcopier/{{ filename }}.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/{{ filename }}.yaml
    - makedirs: True
    - mode: 600
    - template: jinja

{% if files[filename] %}
/etc/cron.d/dbcopier-{{ filename }}:
  file.managed:
    - source: salt://{{ slspath }}/files/cron
    - template: jinja
    - context:
      filename: {{ filename }}
      dow: {{ files[filename] }}

/etc/logrotate.d/dbcopier-{{ filename }}:
  file.managed:
    - source: salt://{{ slspath }}/files/logrotate
    - template: jinja
    - context:
      filename: {{ filename }}
{% endif %}

{% endfor %}
